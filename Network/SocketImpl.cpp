#include "SocketImpl.h"
//#include "SocketProtocol.h"
namespace HS {
	namespace Socket {
		Socket::Socket(asio::io_service &_io_service, std::shared_ptr<HubContext> _parent, bool __isServer) : _socket(_io_service), parent(_parent), ping_deadline(_io_service), read_deadline(_io_service), write_deadline(_io_service), _isServer(__isServer){
			LastOpCode = OpCode::INVALID;
		}
		void Socket::AddMsg(const WSMessage &msg, HS::Socket::CompressionOptions compressmessage) { SendMessageQueue.emplace_back(SendQueueItem{ msg, compressmessage }); }
		void Socket::send(const WSMessage &msg, CompressionOptions compressmessage)
		{
			if (socketStatus == SocketStatus::CONNECTED) { // only send to a connected socket
				auto self(std::static_pointer_cast<Socket>(shared_from_this()));
				sendImpl(self, msg, compressmessage);
			}
		}
		bool Socket::is_loopback() const
		{
			std::error_code ec;
			auto rt(_socket.lowest_layer().remote_endpoint(ec));
			if (!ec)
				return rt.address().is_loopback();
			else
				return true;
		}
		void Socket::canceltimers()
		{
			std::error_code ec;
			read_deadline.cancel(ec);
			ec.clear();
			write_deadline.cancel(ec);
			ec.clear();
			ping_deadline.cancel(ec);
		}
		void Socket::close(unsigned short code, const std::string &msg) {
			if (socketStatus == SocketStatus::CONNECTED) { // only send a close to an open socket
				auto self(std::static_pointer_cast<Socket>(shared_from_this()));
				sendclosemessage(self, code, msg);
			}
		}
		bool Socket::isServer() {
			return _isServer;
		}
	}
}