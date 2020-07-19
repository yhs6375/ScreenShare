#pragma once
#include <memory>
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <deque>
#include <iostream>
#include "SocketProtocol.h"

namespace HS {
	namespace Socket {
		enum SocketStatus : unsigned char { CONNECTING, CONNECTED, CLOSING, CLOSED };
		enum class SocketIOStatus : unsigned char { NOTWRITING, WRITING };
		class HubContext;

		class Socket : public std::enable_shared_from_this<Socket>{
		public:
			asio::ip::tcp::socket _socket;
			SocketStatus socketStatus;
			std::shared_ptr<HubContext> parent;
			unsigned char *ReceiveBuffer = nullptr;
			size_t ReceiveBufferSize = 0;
			unsigned char ReceiveHeader[14] = {};
			SocketIOStatus Writing = SocketIOStatus::NOTWRITING;
			std::deque<SendQueueItem> SendMessageQueue;
			size_t Bytes_PendingFlush = 0;
			OpCode LastOpCode;
			bool FrameCompressed = false;
			asio::basic_waitable_timer<std::chrono::steady_clock> ping_deadline;
			asio::basic_waitable_timer<std::chrono::steady_clock> read_deadline;
			asio::basic_waitable_timer<std::chrono::steady_clock> write_deadline;
			bool _isServer = false;
		public:
			Socket(asio::io_service &_io_service, std::shared_ptr<HubContext> _parent, bool __isServer = false);
			void AddMsg(const WSMessage &msg, HS::Socket::CompressionOptions compressmessage);
			void send(const WSMessage &msg, CompressionOptions compressmessage);
			bool is_loopback() const;
			void canceltimers();
			void close(unsigned short code, const std::string &msg);
			bool isServer();
		};
		const size_t LARGE_BUFFER_SIZE = 1024 * 1024; // 1 MB temp buffer
		struct ThreadContext {
			ThreadContext(asio::ssl::context_base::method m = asio::ssl::context_base::method::tlsv12)
				: work(io_service), context(m)
			{
				thread = std::thread([&] {
					std::error_code ec;
					io_service.run(ec);
				});
			}

			std::thread thread;
			asio::io_service io_service;
			asio::io_service::work work;
			asio::ssl::context context;
		};

		
	}
}