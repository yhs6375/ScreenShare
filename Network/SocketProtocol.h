#pragma once
#include <iostream>
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <random>
#include "../Util/BaseUtil.h"

namespace HS {
	namespace Socket {
		class Socket;
		enum OpCode : unsigned char { CONTINUATION = 0, TEXT = 1, BINARY = 2, CLOSE = 8, PING = 9, PONG = 10, INVALID = 255 };
		enum class CompressionOptions { COMPRESS, NO_COMPRESSION };
		struct WSMessage {
			unsigned char *data;
			size_t len;
			OpCode code;
			std::shared_ptr<unsigned char> Buffer;
		};
		struct SendQueueItem {
			WSMessage msg;
			CompressionOptions compressmessage;
		};
		inline size_t ReadFromExtraData(unsigned char *dst, size_t desired_bytes_to_read, const std::shared_ptr<asio::streambuf> &extradata);
		inline void handleclose(const std::shared_ptr<Socket> &socket, unsigned short code, const std::string &msg);
		template <class SENDBUFFERTYPE>
		void sendImpl(const std::shared_ptr<Socket> &socket, const SENDBUFFERTYPE &msg, CompressionOptions compressmessage);
		inline void startwrite(const std::shared_ptr<Socket> &socket);
		void sendclosemessage(const std::shared_ptr<Socket> &socket, unsigned short code, const std::string &msg);
		inline void ReadBody(const std::shared_ptr<Socket> &socket, const std::shared_ptr<asio::streambuf> &extradata);
		inline void UnMaskMessage(size_t readsize, unsigned char *buffer, bool isserver);
		inline void ProcessMessage(const std::shared_ptr<Socket> &socket, const std::shared_ptr<asio::streambuf> &extradata);
		inline void ProcessControlMessage(const std::shared_ptr<Socket> &socket, const std::shared_ptr<unsigned char> &buffer, size_t size,
			const std::shared_ptr<asio::streambuf> &extradata);
		inline void ProcessClose(const std::shared_ptr<Socket> &socket, const std::shared_ptr<unsigned char> &buffer, size_t size);
		void ReadHeaderNext(const std::shared_ptr<Socket> &socket, const std::shared_ptr<asio::streambuf> &extradata);
		void ReadHeaderStart(const std::shared_ptr<Socket> &socket, const std::shared_ptr<asio::streambuf> &extradata);
		template <class SENDBUFFERTYPE> void write_end(const std::shared_ptr<Socket> &socket, const SENDBUFFERTYPE &msg);
		template <class SENDBUFFERTYPE> void writeend(const std::shared_ptr<Socket> &socket, const SENDBUFFERTYPE &msg, bool iserver);
		inline void SendPong(const std::shared_ptr<Socket> &socket, const std::shared_ptr<unsigned char> &buffer, size_t size);
		inline void ProcessMessageFin(const std::shared_ptr<Socket> &socket, const WSMessage &unpacked, OpCode opcode);
		void writeexpire_from_now(const std::shared_ptr<Socket> &socket, std::chrono::seconds secs);
		template <class SENDBUFFERTYPE> inline void write(const std::shared_ptr<Socket> &socket, const SENDBUFFERTYPE &msg);
		void readexpire_from_now(const std::shared_ptr<Socket> &socket, std::chrono::seconds secs);
		void start_ping(const std::shared_ptr<Socket> &socket, std::chrono::seconds secs);
	}
}