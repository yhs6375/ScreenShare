#pragma once
#include <memory>
#include <asio.hpp>
#include <asio/ssl.hpp>
#include "HeaderParser.h"
#include "SocketProtocol.h"
#include <memory>

#include <deque>
#include "ns.h"
#include "SocketImpl.h"

namespace HS {
	namespace Socket {
		void Listen(const std::shared_ptr<HubContext> &listener);
		void Connect(const std::shared_ptr<HubContext> self, const std::string &host, unsigned int port,
			const std::string &endpoint = "/", const std::unordered_map<std::string, std::string> &extraheaders = {});
		class Socket_Listener {
		private:
			std::shared_ptr<HubContext> _HubContext;
		public:
			
			Socket_Listener(unsigned int port, unsigned short threadCount = 1);
			Socket_Listener(const std::shared_ptr<HubContext> &impl) : _HubContext(impl) {}
			void listen();
			std::shared_ptr<Socket_Listener> onConnection(const std::function<void(const std::shared_ptr<Socket> &, const HttpHeader &)> &handle);
			std::shared_ptr<Socket_Listener> onDisconnection(const ::std::function<void(const std::shared_ptr<Socket> &, unsigned short, const std::string &)> &handle);
			std::shared_ptr<Socket_Listener> onMessage(const std::function<void(const std::shared_ptr<Socket> &, const WSMessage &)> &handle);
		};
		class Socket_Client {
		private:
			std::shared_ptr<HubContext> _HubContext;
		public:
			Socket_Client();
			Socket_Client(const std::shared_ptr<HubContext> &impl);
			void connect(std::string ip);
			std::shared_ptr<Socket_Client> onConnection(const std::function<void(const std::shared_ptr<Socket> &, const HttpHeader &)> &handle);
			std::shared_ptr<Socket_Client> onDisconnection(const std::function<void(const std::shared_ptr<Socket> &, unsigned short, const std::string &)> &handle);
			std::shared_ptr<Socket_Client> onMessage(const std::function<void(const std::shared_ptr<Socket> &, const WSMessage &)> &handle);
			std::shared_ptr<Socket_Client> onPing(const std::function<void(const std::shared_ptr<Socket> &, const unsigned char *, size_t)> &handle);
			std::shared_ptr<Socket_Client> onPong(const std::function<void(const std::shared_ptr<Socket> &, const unsigned char *, size_t)> &handle);
		};
		std::shared_ptr<Socket_Listener> CreateListener(unsigned int port, unsigned short threadCount = 1);
		std::shared_ptr<Socket_Client> CreateClient();
		
	}
}