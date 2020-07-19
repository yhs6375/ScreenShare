#include "SocketProtocol.h"
#include "SocketImpl.h"
#include "HubContext.h"
namespace HS {
	namespace Socket {
		inline size_t ReadFromExtraData(unsigned char *dst, size_t desired_bytes_to_read, const std::shared_ptr<asio::streambuf> &extradata)
		{
			size_t dataconsumed = 0;
			if (extradata->size() >= desired_bytes_to_read) {
				dataconsumed = desired_bytes_to_read;
			}
			else {
				dataconsumed = extradata->size();
			}
			if (dataconsumed > 0) {
				desired_bytes_to_read -= dataconsumed;
				memcpy(dst, asio::buffer_cast<const void *>(extradata->data()), dataconsumed);
				extradata->consume(dataconsumed);
			}
			return dataconsumed;
		}
		inline void handleclose(const std::shared_ptr<Socket> &socket, unsigned short code, const std::string &msg)
		{
			socket->socketStatus = SocketStatus::CLOSED;
			socket->Writing = SocketIOStatus::NOTWRITING;
			if (socket->parent->onDisconnection) {
				socket->parent->onDisconnection(socket, code, msg);
			}

			socket->SendMessageQueue.clear(); // clear all outbound messages
			socket->canceltimers();
			std::error_code ec;
			socket->_socket.lowest_layer().shutdown(asio::ip::tcp::socket::shutdown_both, ec);
			ec.clear();
			socket->_socket.lowest_layer().close(ec);
		}
		template <class SENDBUFFERTYPE> void write_end(const std::shared_ptr<Socket> &socket, const SENDBUFFERTYPE &msg)
		{

			asio::async_write(socket->_socket, asio::buffer(msg.data, msg.len), [socket, msg](const std::error_code &ec, size_t bytes_transferred) {
				socket->Writing = SocketIOStatus::NOTWRITING;
				socket->Bytes_PendingFlush -= msg.len;
				if (msg.code == OpCode::CLOSE) {
					// final close.. get out and dont come back mm kay?
					return handleclose(socket, 1000, "");
				}
				if (ec) {
					return handleclose(socket, 1002, "write header failed " + ec.message());
				}
				assert(msg.len == bytes_transferred);
				startwrite(socket);
			});
		}
		template <class SENDBUFFERTYPE> void writeend(const std::shared_ptr<Socket> &socket, const SENDBUFFERTYPE &msg, bool iserver)
		{
			if (!iserver) {
				std::uniform_int_distribution<unsigned int> dist(0, 255);
				std::random_device rd;

				unsigned char mask[4];
				for (auto c = 0; c < 4; c++) {
					mask[c] = static_cast<unsigned char>(dist(rd));
				}
				auto p = reinterpret_cast<unsigned char *>(msg.data);
				for (decltype(msg.len) i = 0; i < msg.len; i++) {
					*p++ ^= mask[i % 4];
				}
				std::error_code ec;
				auto bytes_transferred = asio::write(socket->_socket, asio::buffer(mask, 4), ec);
				if (ec) {
					if (msg.code == OpCode::CLOSE) {
						return handleclose(socket, msg.code, "");
					}
					else {
						return handleclose(socket, 1002, "write mask failed " + ec.message());
					}
				}
				else {
					assert(bytes_transferred == 4);
					write_end(socket, msg);
				}
			}
			else {
				write_end(socket, msg);
			}
		}
		inline void SendPong(const std::shared_ptr<Socket> &socket, const std::shared_ptr<unsigned char> &buffer, size_t size)
		{
			WSMessage msg;
			msg.Buffer = buffer;
			msg.len = size;
			msg.code = OpCode::PONG;
			msg.data = msg.Buffer.get();

			sendImpl(socket, msg, CompressionOptions::NO_COMPRESSION);
		}
		inline void UnMaskMessage(size_t readsize, unsigned char *buffer, bool isserver)
		{
			if (isserver) {
				auto startp = buffer;
				unsigned char mask[4] = {};
				memcpy(mask, startp, 4);
				for (size_t c = 4; c < readsize; c++) {
					startp[c - 4] = startp[c] ^ mask[c % 4];
				}
			}
		}
		inline void ProcessMessageFin(const std::shared_ptr<Socket> &socket, const WSMessage &unpacked, OpCode opcode)
		{
			if (socket->LastOpCode == OpCode::TEXT || opcode == OpCode::TEXT) {
				if (!Util::isValidUtf8(unpacked.data, unpacked.len)) {
					return sendclosemessage(socket, 1007, "Frame not valid utf8");
				}
			}
			if (socket->parent->onMessage) {
				socket->parent->onMessage(socket, unpacked);
			}
		}
		inline void ProcessMessage(const std::shared_ptr<Socket> &socket, const std::shared_ptr<asio::streambuf> &extradata)
		{

			auto opcode = static_cast<OpCode>(Util::getOpCode(socket->ReceiveHeader));

			if (!Util::getFin(socket->ReceiveHeader)) {
				if (socket->LastOpCode == OpCode::INVALID) {
					if (opcode != OpCode::BINARY && opcode != OpCode::TEXT) {
						return sendclosemessage(socket, 1002, "First Non Fin Frame must be binary or text");
					}
					socket->LastOpCode = opcode;
				}
				else if (opcode != OpCode::CONTINUATION) {
					return sendclosemessage(socket, 1002, "Continuation Received without a previous frame");
				}
				return ReadHeaderNext(socket, extradata);
			}
			else {
				if (socket->LastOpCode != OpCode::INVALID && opcode != OpCode::CONTINUATION) {
					return sendclosemessage(socket, 1002, "Continuation Received without a previous frame");
				}
				else if (socket->LastOpCode == OpCode::INVALID && opcode == OpCode::CONTINUATION) {
					return sendclosemessage(socket, 1002, "Continuation Received without a previous frame");
				}

				// this could be compressed.... lets check it out
				if (socket->FrameCompressed || Util::getrsv1(socket->ReceiveHeader)) { // is this the last of the messages? Decompress!!!
					socket->parent->beginInflate();
					auto[buffer, buffer_length] = socket->parent->Inflate(socket->ReceiveBuffer, socket->ReceiveBufferSize);
					auto unpacked = WSMessage{ buffer, buffer_length, socket->LastOpCode != OpCode::INVALID ? socket->LastOpCode : opcode };
					ProcessMessageFin(socket, unpacked, opcode);
					socket->parent->endInflate();
				}
				else {
					auto unpacked =
						WSMessage{ socket->ReceiveBuffer, socket->ReceiveBufferSize, socket->LastOpCode != OpCode::INVALID ? socket->LastOpCode : opcode };
					ProcessMessageFin(socket, unpacked, opcode);
				}
				ReadHeaderStart(socket, extradata);
			}
		}
		inline void ProcessClose(const std::shared_ptr<Socket> &socket, const std::shared_ptr<unsigned char> &buffer, size_t size)
		{
			if (size >= 2) {
				auto closecode = Util::hton(*reinterpret_cast<unsigned short *>(buffer.get()));
				if (size > 2) {
					if (!Util::isValidUtf8(buffer.get() + sizeof(closecode), size - sizeof(closecode))) {
						return sendclosemessage(socket, 1007, "Frame not valid utf8");
					}
				}

				if (((closecode >= 1000 && closecode <= 1011) || (closecode >= 3000 && closecode <= 4999)) && closecode != 1004 && closecode != 1005 &&
					closecode != 1006) {
					return sendclosemessage(socket, 1000, "");
				}
				else {
					return sendclosemessage(socket, 1002, "");
				}
			}
			else if (size != 0) {
				return sendclosemessage(socket, 1002, "");
			}
			return sendclosemessage(socket, 1000, "");
		}
		inline void ProcessControlMessage(const std::shared_ptr<Socket> &socket, const std::shared_ptr<unsigned char> &buffer, size_t size,
			const std::shared_ptr<asio::streambuf> &extradata)
		{
			if (!Util::getFin(socket->ReceiveHeader)) {
				return sendclosemessage(socket, 1002, "Closing connection. Control Frames must be Fin");
			}
			auto opcode = static_cast<OpCode>(Util::getOpCode(socket->ReceiveHeader));

			switch (opcode) {
			case OpCode::PING:
				if (socket->parent->onPing) {
					socket->parent->onPing(socket, buffer.get(), size);
				}
				SendPong(socket, buffer, size);
				break;
			case OpCode::PONG:
				if (socket->parent->onPong) {
					socket->parent->onPong(socket, buffer.get(), size);
				}
				break;
			case OpCode::CLOSE:
				return ProcessClose(socket, buffer, size);

			default:
				return sendclosemessage(socket, 1002, "Closing connection. nonvalid op code");
			}
			ReadHeaderNext(socket, extradata);
		}
		inline void ReadBody(const std::shared_ptr<Socket> &socket, const std::shared_ptr<asio::streambuf> &extradata)
		{
			if (!Util::DidPassMaskRequirement(socket->ReceiveHeader, socket->isServer())) { // Close connection if it did not meet the mask requirement.
				return sendclosemessage(socket, 1002, "Closing connection because mask requirement not met");
			}
			if (Util::getrsv2(socket->ReceiveHeader) || Util::getrsv3(socket->ReceiveHeader) ||
				(Util::getrsv1(socket->ReceiveHeader))) {
				return sendclosemessage(socket, 1002, "Closing connection. rsv bit set");
			}
			auto opcode = static_cast<OpCode>(Util::getOpCode(socket->ReceiveHeader));

			size_t size = Util::getpayloadLength1(socket->ReceiveHeader);
			switch (size) {
			case 126:
				size = Util::ntoh(Util::getpayloadLength2(socket->ReceiveHeader));
				break;
			case 127:
				size = static_cast<size_t>(Util::ntoh(Util::getpayloadLength8(socket->ReceiveHeader)));
				if (size > std::numeric_limits<std::size_t>::max()) {
					return sendclosemessage(socket, 1009, "Payload exceeded MaxPayload size");
				}
				break;
			default:
				break;
			}

			size += Util::AdditionalBodyBytesToRead(socket->isServer());
			if (opcode == OpCode::PING || opcode == OpCode::PONG || opcode == OpCode::CLOSE) {
				if (size - Util::AdditionalBodyBytesToRead(socket->isServer()) > Util::CONTROLBUFFERMAXSIZE) {
					return sendclosemessage(socket, 1002, "Payload exceeded for control frames. Size requested " + std::to_string(size));
				}
				else if (size > 0) {
					auto buffer = std::shared_ptr<unsigned char>(new unsigned char[size], [](unsigned char *p) { delete[] p; });

					auto bytestoread = size;
					auto dataconsumed = ReadFromExtraData(buffer.get(), bytestoread, extradata);
					bytestoread -= dataconsumed;

					asio::async_read(socket->_socket, asio::buffer(buffer.get() + dataconsumed, bytestoread),
						[size, extradata, socket, buffer](const std::error_code &ec, size_t) {
						if (!ec) {
							UnMaskMessage(size, buffer.get(), socket->isServer());
							auto tempsize = size - Util::AdditionalBodyBytesToRead(socket->isServer());
							return ProcessControlMessage(socket, buffer, tempsize, extradata);
						}
						else {
							return sendclosemessage(socket, 1002, "ReadBody Error " + ec.message());
						}
					});
				}
				else {
					std::shared_ptr<unsigned char> ptr;
					return ProcessControlMessage(socket, ptr, 0, extradata);
				}
			}

			else if (opcode == OpCode::TEXT || opcode == OpCode::BINARY || opcode == OpCode::CONTINUATION) {
				auto addedsize = socket->ReceiveBufferSize + size;
				if (addedsize > std::numeric_limits<std::size_t>::max()) {
					return sendclosemessage(socket, 1009, "Payload exceeded MaxPayload size");
				}
				socket->ReceiveBufferSize = addedsize;

				if (socket->ReceiveBufferSize > socket->parent->MaxPayload) {
					return sendclosemessage(socket, 1009, "Payload exceeded MaxPayload size");
				}
				if (socket->ReceiveBufferSize > std::numeric_limits<std::size_t>::max()) {
					return sendclosemessage(socket, 1009, "Payload exceeded MaxPayload size");
				}

				if (size > 0) {
					printf("\nsocket->ReceiveBuffer!!!\n");
					socket->ReceiveBuffer = static_cast<unsigned char *>(realloc(socket->ReceiveBuffer, socket->ReceiveBufferSize));
					if (!socket->ReceiveBuffer) {
						return sendclosemessage(socket, 1009, "Payload exceeded MaxPayload size");
					}

					auto bytestoread = size;
					auto dataconsumed = ReadFromExtraData(socket->ReceiveBuffer + socket->ReceiveBufferSize - size, bytestoread, extradata);
					bytestoread -= dataconsumed;
					asio::async_read(socket->_socket, asio::buffer(socket->ReceiveBuffer + socket->ReceiveBufferSize - size + dataconsumed, bytestoread),
						[size, extradata, socket](const std::error_code &ec, size_t) {
						if (!ec) {
							auto buffer = socket->ReceiveBuffer + socket->ReceiveBufferSize - size;
							UnMaskMessage(size, buffer, socket->isServer());
							socket->ReceiveBufferSize -= Util::AdditionalBodyBytesToRead(socket->isServer());
							return ProcessMessage(socket, extradata);
						}
						else {
							return sendclosemessage(socket, 1002, "ReadBody Error " + ec.message());
						}
					});
				}
				else {
					return ProcessMessage(socket, extradata);
				}
			}
			else {
				return sendclosemessage(socket, 1002, "Closing connection. nonvalid op code");
			}
		}
		void sendclosemessage(const std::shared_ptr<Socket> &socket, unsigned short code, const std::string &msg)
		{
			WSMessage ws;
			ws.code = OpCode::CLOSE;
			ws.len = sizeof(code) + msg.size();
			ws.Buffer = std::shared_ptr<unsigned char>(new unsigned char[ws.len], [](unsigned char *p) { delete[] p; });
			*reinterpret_cast<unsigned short *>(ws.Buffer.get()) = Util::ntoh(code);
			memcpy(ws.Buffer.get() + sizeof(code), msg.c_str(), msg.size());
			ws.data = ws.Buffer.get();
			sendImpl(socket, ws, CompressionOptions::NO_COMPRESSION);
		}
		void writeexpire_from_now(const std::shared_ptr<Socket> &socket, std::chrono::seconds secs)
		{

			std::error_code ec;
			if (secs.count() == 0)
				socket->write_deadline.cancel(ec);
			socket->write_deadline.expires_from_now(secs, ec);
			if (ec) {
				std::cout << ec.message() << std::endl;
			}
			else if (secs.count() > 0) {
				socket->write_deadline.async_wait([socket](const std::error_code &ec) {
					if (ec != asio::error::operation_aborted) {
						return sendclosemessage(socket, 1001, "write timer expired on the socket ");
					}
				});
			}
		}
		template <class SENDBUFFERTYPE> inline void write(const std::shared_ptr<Socket> &socket, const SENDBUFFERTYPE &msg)
		{
			size_t sendsize = 0;
			unsigned char header[10] = {};

			Util::setFin(header, 0xFF);
			Util::set_MaskBitForSending(header, socket->isServer());
			Util::setOpCode(header, msg.code);
			Util::setrsv1(header, 0x00);
			Util::setrsv2(header, 0x00);
			Util::setrsv3(header, 0x00);

			if (msg.len <= 125) {
				Util::setpayloadLength1(header, Util::hton(static_cast<unsigned char>(msg.len)));
				sendsize = 2;
			}
			else if (msg.len > USHRT_MAX) {
				Util::setpayloadLength8(header, Util::hton(static_cast<unsigned long long int>(msg.len)));
				Util::setpayloadLength1(header, 127);
				sendsize = 10;
			}
			else {
				Util::setpayloadLength2(header, Util::hton(static_cast<unsigned short>(msg.len)));
				Util::setpayloadLength1(header, 126);
				sendsize = 4;
			}

			assert(msg.len < UINT32_MAX);
			writeexpire_from_now(socket, std::chrono::seconds(30));
			std::error_code ec;
			auto bytes_transferred = asio::write(socket->_socket, asio::buffer(header, sendsize), ec);
			if (!ec) {
				assert(sendsize == bytes_transferred);
				writeend(socket, msg, socket->isServer());
			}
			else {
				handleclose(socket, 1002, "write header failed " + ec.message());
			}
		}
		inline void startwrite(const std::shared_ptr<Socket> &socket)
		{
			if (socket->Writing == SocketIOStatus::NOTWRITING) {
				if (!socket->SendMessageQueue.empty()) {
					socket->Writing = SocketIOStatus::WRITING;
					auto msg(socket->SendMessageQueue.front());
					socket->SendMessageQueue.pop_front();
					write(socket, msg.msg);
				}
				else {
					writeexpire_from_now(socket, std::chrono::seconds(0)); // make sure the write timer doesnt kick off
				}
			}
		}
		template <class SENDBUFFERTYPE>
		void sendImpl(const std::shared_ptr<Socket> &socket, const SENDBUFFERTYPE &msg, CompressionOptions compressmessage)
		{
			if (compressmessage == CompressionOptions::COMPRESS) {
				assert(msg.code == OpCode::BINARY || msg.code == OpCode::TEXT);
			}
			asio::post(socket->_socket.get_io_service(), [socket, msg, compressmessage]() {

				if (socket->socketStatus == SocketStatus::CONNECTED) {
					// update the socket status to reflect it is closing to prevent other messages from being sent.. this is the last valid message
					// make sure to do this after a call to write so the write process sends the close message, but no others
					if (msg.code == OpCode::CLOSE) {
						socket->socketStatus = SocketStatus::CLOSING;
					}
					socket->Bytes_PendingFlush += msg.len;
					socket->AddMsg(msg, compressmessage);
					startwrite(socket);
				}
			});
		}
		void readexpire_from_now(const std::shared_ptr<Socket> &socket, std::chrono::seconds secs)
		{
			std::error_code ec;
#ifdef _DEBUG
			socket->read_deadline.cancel(ec);
			secs = std::chrono::seconds(0);
#elif
			if (secs.count() == 0)
				socket->read_deadline.cancel(ec);
#endif
			socket->read_deadline.expires_from_now(secs, ec);
			if (ec) {
				std::cout << ec.message() << std::endl;
			}
			else if (secs.count() > 0) {
				socket->read_deadline.async_wait([socket](const std::error_code &ec) {
					if (ec != asio::error::operation_aborted) {
						return sendclosemessage(socket, 1001, "read timer expired on the socket ");
					}
				});
			}
		}
		void ReadHeaderNext(const std::shared_ptr<Socket> &socket, const std::shared_ptr<asio::streambuf> &extradata)
		{
			readexpire_from_now(socket, std::chrono::seconds(30));
			size_t bytestoread = 2;
			auto dataconsumed = ReadFromExtraData(socket->ReceiveHeader, bytestoread, extradata);
			bytestoread -= dataconsumed;

			asio::async_read(
				socket->_socket, asio::buffer(socket->ReceiveHeader + dataconsumed, bytestoread), [socket, extradata](const std::error_code &ec, size_t) {
				if (!ec) {

					size_t bytestoread = Util::getpayloadLength1(socket->ReceiveHeader);
					switch (bytestoread) {
					case 126:
						bytestoread = 2;
						break;
					case 127:
						bytestoread = 8;
						break;
					default:
						bytestoread = 0;
					}
					if (bytestoread > 1) {
						auto dataconsumed = ReadFromExtraData(socket->ReceiveHeader + 2, bytestoread, extradata);
						bytestoread -= dataconsumed;

						asio::async_read(socket->_socket, asio::buffer(socket->ReceiveHeader + 2 + dataconsumed, bytestoread),
							[socket, extradata](const std::error_code &ec, size_t) {
							if (!ec) {
								ReadBody(socket, extradata);
							}
							else {
								return sendclosemessage(socket, 1002, "readheader ExtendedPayloadlen " + ec.message());
							}
						});
					}
					else {
						ReadBody(socket, extradata);
					}
				}
				else {
					return sendclosemessage(socket, 1002, "WebSocket ReadHeader failed " + ec.message());
				}
			});
		}
		void ReadHeaderStart(const std::shared_ptr<Socket> &socket, const std::shared_ptr<asio::streambuf> &extradata)
		{
			free(socket->ReceiveBuffer);
			socket->ReceiveBuffer = nullptr;
			socket->ReceiveBufferSize = 0;
			socket->LastOpCode = OpCode::INVALID;
			socket->FrameCompressed = false;
			ReadHeaderNext(socket, extradata);
		}
		void start_ping(const std::shared_ptr<Socket> &socket, std::chrono::seconds secs)
		{
			std::error_code ec;
#ifdef _DEBUG
			socket->ping_deadline.cancel(ec);
			secs = std::chrono::seconds(0);
#elif
			if (secs.count() == 0)
				socket->ping_deadline.cancel(ec);
#endif
			socket->ping_deadline.expires_from_now(secs, ec);
			if (ec) {
				std::cout << "Socket.cpp start_ping error" << std::endl;
			}
			else if (secs.count() > 0) {
				socket->ping_deadline.async_wait([socket, secs](const std::error_code &ec) {
					if (ec != asio::error::operation_aborted) {
						WSMessage msg;
						char p[] = "ping";
						msg.Buffer = std::shared_ptr<unsigned char>(new unsigned char[sizeof(p)], [](unsigned char *ptr) { delete[] ptr; });
						memcpy(msg.Buffer.get(), p, sizeof(p));
						msg.len = sizeof(p);
						msg.code = OpCode::PING;
						msg.data = msg.Buffer.get();
						sendImpl(socket, msg, CompressionOptions::NO_COMPRESSION);
						start_ping(socket, secs);
					}
				});
			}
		}
	}
}