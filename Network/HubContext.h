#pragma once
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <zlib.h>
#include "HeaderParser.h"
namespace HS {
	namespace Socket {
		class Socket;
		struct WSMessage;
		struct ThreadContext;
		class HubContext {
		public:
			std::function<void(const std::shared_ptr<Socket> &, const HttpHeader &)> onConnection;
			std::function<void(const std::shared_ptr<Socket> &, unsigned short, const std::string &)> onDisconnection;
			std::function<void(const std::shared_ptr<Socket> &, const WSMessage &)> onMessage;
			std::function<void(const std::shared_ptr<Socket> &, const unsigned char *, size_t)> onPing;
			std::function<void(const std::shared_ptr<Socket> &, const unsigned char *, size_t)> onPong;
			std::vector<std::shared_ptr<ThreadContext>> ThreadContexts;
			std::unique_ptr<asio::ip::tcp::acceptor> acceptor;
			std::atomic<std::size_t> m_nextService{ 0 };
			size_t MaxPayload = 1024 * 1024 * 20;
			unsigned char *InflateBuffer = nullptr;
			size_t InflateBufferSize = 0;
			z_stream InflationStream = {};
			std::unique_ptr<unsigned char[]> TempInflateBuffer;
		public:
			HubContext(unsigned short threadCount = 1);
			std::shared_ptr<ThreadContext> getnextContext();
			void beginInflate();
			std::tuple<unsigned char *, size_t> returnemptyinflate();
			std::tuple<unsigned char *, size_t> Inflate(unsigned char *data, size_t data_len);
			void endInflate();
		};
	}
}