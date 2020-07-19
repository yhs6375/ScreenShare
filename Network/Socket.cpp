#include "Socket.h"
#include "HubContext.h"
namespace HS {
	namespace Socket{
		struct HandshakeContainer {
			asio::streambuf Read;
			std::string Write;
			HttpHeader Header;
		};

		Socket_Listener::Socket_Listener(unsigned int port, unsigned short threadCount) {
			_HubContext = std::make_shared<HubContext>(threadCount);
			_HubContext->acceptor = std::make_unique<asio::ip::tcp::acceptor>(_HubContext->getnextContext()->io_service, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port));
		}
		void Socket_Listener::listen() {
			Listen(_HubContext);

		}
		std::shared_ptr<Socket_Listener> Socket_Listener::onConnection(const std::function<void(const std::shared_ptr<Socket> &, const HttpHeader &)> &handle) {
			_HubContext->onConnection = handle;
			return std::make_shared<Socket_Listener>(_HubContext);
		}
		std::shared_ptr<Socket_Listener> Socket_Listener::onDisconnection(const ::std::function<void(const std::shared_ptr<Socket> &, unsigned short, const std::string &)> &handle) {
			_HubContext->onDisconnection = handle;
			return std::make_shared<Socket_Listener>(_HubContext);
		}
		std::shared_ptr<Socket_Listener>
			Socket_Listener::onMessage(const std::function<void(const std::shared_ptr<Socket> &, const WSMessage &)> &handle)
		{
			_HubContext->onMessage = handle;
			return std::make_shared<Socket_Listener>(_HubContext);
		}
		
		void async_handshake(const std::shared_ptr<HubContext> listener, const std::shared_ptr<Socket> socket);
		void read_handshake(const std::shared_ptr<HubContext> listener, const std::shared_ptr<Socket> socket);
		template <bool isServer> void start_ping(const std::shared_ptr<Socket> &socket, std::chrono::seconds secs);
		template <typename T> auto CreateHandShake(T &header)
		{
			auto header_it =
				std::find_if(std::begin(header.Values), std::end(header.Values), [](HeaderKeyValue k) { return k.Key == "Sec-WebSocket-Key"; });
			if (header_it == header.Values.end()) {
				return std::make_tuple(std::string(), false);
			}

			std::string shastr(header_it->Value.data(), header_it->Value.size());
			shastr += ws_magic_string;
			auto sha1 = Util::SHA1(shastr);

			std::string str = "HTTP/1.1 101 Switching Protocols\r\n";
			str += "Upgrade:websocket\r\n";
			str += "Connection:Upgrade\r\n";
			str += "Sec-WebSocket-Accept:" + Util::Base64encode(sha1) + "\r\n";

			return std::make_tuple(str, true);
		}
		
		void Listen(const std::shared_ptr<HubContext> &listener) {
			auto res = listener->getnextContext();
			auto socket = std::make_shared<Socket>(res->io_service, listener, true);
			socket->socketStatus = HS::Socket::SocketStatus::CONNECTING;
			listener->acceptor->async_accept(socket->_socket.lowest_layer(),
				[listener, socket](const std::error_code &ec) {
				std::cout << socket->_socket.local_endpoint().address().to_string() << std::endl;
				std::cout << socket->_socket.local_endpoint().port() << std::endl;
				std::cout << socket->_socket.remote_endpoint().address().to_string() << std::endl;
				std::cout << socket->_socket.remote_endpoint().port() << std::endl;
				std::error_code e;
				socket->_socket.lowest_layer().set_option(asio::socket_base::reuse_address(true), e);
				socket->_socket.lowest_layer().set_option(asio::ip::tcp::no_delay(true), e);
				if (!ec) {
					async_handshake(listener, socket);
				}
				else {
					socket->socketStatus = HS::Socket::SocketStatus::CLOSED;
				}
				Listen(listener);
			}
			);
		}
		void async_handshake(const std::shared_ptr<HubContext> listener, const std::shared_ptr<Socket> socket) {
			read_handshake(listener, socket);
		}
		void read_handshake(const std::shared_ptr<HubContext> listener, const std::shared_ptr<Socket> socket) {
			auto handshakecontainer(std::make_shared<HandshakeContainer>());
			asio::async_read_until(socket->_socket,handshakecontainer->Read,"\r\n\r\n",
				[listener, socket, handshakecontainer](const std::error_code &ec, size_t bytes_transferred) {
				if (!ec) {
					std::cout << "Read Handshake bytes " << bytes_transferred << std::endl;
					handshakecontainer->Header = ParseHeader(asio::buffer_cast<const char *>(handshakecontainer->Read.data()));
					if (auto[response, parsesuccess] = CreateHandShake(handshakecontainer->Header); parsesuccess) {
						handshakecontainer->Write = response;
						handshakecontainer->Write += "\r\n";
						asio::async_write(socket->_socket, asio::buffer(handshakecontainer->Write.data(), handshakecontainer->Write.size()),
							[listener, socket, handshakecontainer](const std::error_code &ec, size_t bytes_transferred) {
							if (!ec) {
								std::cout << "Connected: Sent Handshake bytes" << bytes_transferred << std::endl;
								socket->socketStatus = SocketStatus::CONNECTED;
								if (listener->onConnection) {
									listener->onConnection(socket, handshakecontainer->Header);
								}
								auto bufptr = std::make_shared<asio::streambuf>();
								ReadHeaderStart(socket, bufptr);
								start_ping(socket, std::chrono::seconds(5));
							}
							else {
								socket->socketStatus = SocketStatus::CLOSED;
								std::cout << "Socket receivehandshake failed" << std::endl;
							}
						});
					}
				}
			});
		}
		

		Socket_Client::Socket_Client() {
			_HubContext = std::make_shared<HubContext>(1);
		}
		Socket_Client::Socket_Client(const std::shared_ptr<HubContext> &impl){
			_HubContext = impl;
		}
		void Socket_Client::connect(std::string ip) {
			HS::Socket::Connect(_HubContext, ip, 6001);
		}
		std::shared_ptr<Socket_Client> Socket_Client::onConnection(const std::function<void(const std::shared_ptr<Socket> &, const HttpHeader &)> &handle) {
			_HubContext->onConnection = handle;
			return std::make_shared<Socket_Client>(_HubContext);
		}
		std::shared_ptr<Socket_Client> Socket_Client::onDisconnection(const std::function<void(const std::shared_ptr<Socket> &, unsigned short, const std::string &)> &handle) {
			_HubContext->onDisconnection = handle;
			return std::make_shared<Socket_Client>(_HubContext);
		}
		std::shared_ptr<Socket_Client>
			Socket_Client::onMessage(const std::function<void(const std::shared_ptr<Socket> &, const WSMessage &)> &handle)
		{
			_HubContext->onMessage = handle;
			return std::make_shared<Socket_Client>(_HubContext);
		}
		std::shared_ptr<Socket_Client> Socket_Client::onPing(const std::function<void(const std::shared_ptr<Socket> &, const unsigned char *, size_t)> &handle) {
			_HubContext->onPing = handle;
			return std::make_shared<Socket_Client>(_HubContext);
		}
		std::shared_ptr<Socket_Client> Socket_Client::onPong(const std::function<void(const std::shared_ptr<Socket> &, const unsigned char *, size_t)> &handle)
		{
			_HubContext->onPong = handle;
			return std::make_shared<Socket_Client>(_HubContext);
		}

		std::shared_ptr<Socket_Listener> CreateListener(unsigned int port, unsigned short threadCount) {
			return std::make_shared<Socket_Listener>(port, threadCount);
		}
		std::shared_ptr<Socket_Client> CreateClient() {
			return std::make_shared<Socket_Client>();
		}
	}
}