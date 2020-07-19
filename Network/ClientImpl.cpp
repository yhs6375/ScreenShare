#include "ClientImpl.h"
#include "HubContext.h"

namespace HS {
	namespace Socket {
		void ConnectHandshake(const std::shared_ptr<HubContext> self, std::shared_ptr<Socket> &socket, const std::string &host, const std::string &endpoint,
			const std::unordered_map<std::string, std::string> &extraheaders)
		{
			auto write_buffer(std::make_shared<asio::streambuf>());
			std::ostream request(write_buffer.get());

			request << "GET " << endpoint << " HTTP/1.1\r\n";
			request << "Host:" << host << "\r\n";
			request << "Upgrade: websocket\r\n";
			request << "Connection: Upgrade\r\n";

			// Make random 16-byte nonce
			std::string nonce;
			nonce.resize(16);
			std::uniform_int_distribution<unsigned int> dist(0, 255);
			std::random_device rd;
			for (int c = 0; c < 16; c++) {
				nonce[c] = static_cast<unsigned char>(dist(rd));
			}

			auto nonce_base64 = Util::Base64encode(nonce);
			request << "Sec-WebSocket-Key:" << nonce_base64 << "\r\n";
			request << "Sec-WebSocket-Version: 13\r\n";
			for (auto &h : extraheaders) {
				request << h.first << ":" << h.second << "\r\n";
			}
			//  request << "" << HTTP_ENDLINE;
			//  request << HTTP_SECWEBSOCKETEXTENSIONS << HTTP_KEYVALUEDELIM << PERMESSAGEDEFLATE << HTTP_ENDLINE;
			request << "\r\n";

			auto accept_sha1 = Util::SHA1(nonce_base64 + ws_magic_string);

			asio::async_write(
				socket->_socket, *write_buffer, [write_buffer, accept_sha1, socket, self](const std::error_code &ec, size_t bytes_transferred) {
				if (!ec) {
					std::cout<<"Sent Handshake bytes " << bytes_transferred << std::endl;
					auto read_buffer(std::make_shared<asio::streambuf>());
					asio::async_read_until(socket->_socket, *read_buffer, "\r\n\r\n",
						[read_buffer, accept_sha1, socket, self](const std::error_code &ec, size_t bytes_transferred) {
						if (!ec) {
							std::cout << "Read Handshake bytes " << bytes_transferred
								<< "  sizeof read_buffer "
								<< read_buffer->size() << std::endl;

							auto header = ParseHeader(asio::buffer_cast<const char *>(read_buffer->data()));
							auto sockey = std::find_if(std::begin(header.Values), std::end(header.Values),
								[](HeaderKeyValue k) { return k.Key == "Sec-WebSocket-Accept"; });

							if (sockey == std::end(header.Values)) {
								return;
							}
							if (Util::Base64decode(sockey->Value) == accept_sha1) {

								std::cout << "Connected " << std::endl;
								socket->socketStatus = SocketStatus::CONNECTED;
								start_ping(socket, std::chrono::seconds(5));
								if (socket->parent->onConnection) {
									socket->parent->onConnection(socket, header);
								}
								auto bufptr = std::make_shared<asio::streambuf>();
								ReadHeaderStart(socket, bufptr);
							}
							else {
								socket->socketStatus = SocketStatus::CLOSED;
								std::cout << "WebSocket handshake failed  " << std::endl;
								if (socket->parent->onDisconnection) {
									socket->parent->onDisconnection(socket, 1002, "WebSocket handshake failed  ");
								}
							}
						}
						else {
							socket->socketStatus = SocketStatus::CLOSED;
							std::cout << "async_read_until failed  " << ec.message() << std::endl;
							if (socket->parent->onDisconnection) {
								socket->parent->onDisconnection(socket, 1002, "async_read_until failed  " + ec.message());
							}
						}
					});
				}
				else {
					socket->socketStatus = SocketStatus::CLOSED;
					std::cout << "Failed sending handshake" << ec.message() << std::endl;
					if (socket->parent->onDisconnection) {
						socket->parent->onDisconnection(socket, 1002, "Failed sending handshake" + ec.message());
					}
				}
			});
		}
		void async_handshake(const std::shared_ptr<HubContext> self, std::shared_ptr<Socket> socket,
			const std::string &host, const std::string &endpoint, const std::unordered_map<std::string, std::string> &extraheaders)
		{
			ConnectHandshake(self, socket, host, endpoint, extraheaders);
		}
		void Connect(const std::shared_ptr<HubContext> self, const std::string &host, unsigned int port,
			const std::string &endpoint, const std::unordered_map<std::string, std::string> &extraheaders)
		{
			auto res = self->getnextContext();
			auto socket = std::make_shared<Socket>(res->io_service, self);
			socket->socketStatus = SocketStatus::CONNECTING;

			auto portstr = std::to_string(port);
			asio::ip::tcp::resolver::query query(host, portstr.c_str());

			auto resolver = std::make_shared<asio::ip::tcp::resolver>(socket->_socket.get_io_service());
			auto connected = std::make_shared<bool>(false);
			resolver->async_resolve(query, [socket, self, host, endpoint, extraheaders, resolver,
				connected](const std::error_code &ec, asio::ip::tcp::resolver::iterator it) {
				asio::ip::tcp::endpoint end = *it;
				std::cout << end.address() << ' ';
				printf("¾Æ¾Æ\n");
				if (*connected)
					return; // done

				asio::async_connect(
					socket->_socket.lowest_layer(), it,
					[socket, self, host, endpoint, extraheaders, connected](const std::error_code &ec, asio::ip::tcp::resolver::iterator) {
					*connected = true;
					std::error_code e;
					socket->_socket.lowest_layer().set_option(asio::ip::tcp::no_delay(true), e);
					if (e) {
						std::cout << "set_option error " << e.message() << std::endl;
						e.clear();
					}
					if (!ec) {
						async_handshake(self, socket, host, endpoint, extraheaders);
					}
					else {
						std::cout << "Failed async_connect " << ec.message() << std::endl;
						socket->socketStatus = SocketStatus::CLOSED;
						if (socket->parent->onDisconnection) {
							socket->parent->onDisconnection(socket, 1002, "Failed async_connect " + ec.message());
						}
					}
				});
			});
		}
	}
}
