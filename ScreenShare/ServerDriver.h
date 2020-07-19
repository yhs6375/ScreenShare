#pragma once
#include <atomic>
#include <shared_mutex>

#include "IRATServer.h"
#include "RAT.h"
#include <ScreenCapture.h>
#include <internal/SCCommon.h>
#include <turbojpeg.h>
#include "Input/InputLite.h"
namespace HS {
	namespace RAT_Server {
		
		class RATServer : public IRATServer{
		private:
			void MouseImageChanged(const std::shared_ptr<Socket::Socket> &socket, const unsigned char *data, size_t len);
			void MousePositionChanged(const std::shared_ptr<Socket::Socket> &socket, const unsigned char *data, size_t len);
			void MonitorsChanged(const std::shared_ptr<Socket::Socket> &socket, const unsigned char *data, size_t len);
			void ClipboardTextChanged(const std::shared_ptr<Socket::Socket> &socket, const unsigned char *data, size_t len);
			template <typename CALLBACKTYPE> void Frame(const std::shared_ptr<Socket::Socket> &socket, const unsigned char *data, size_t len, CALLBACKTYPE &&cb);
		public:
			
			std::atomic<int> ClientCount;
			int MaxNumConnections = 10;

			std::mutex outputbufferLock;
			std::vector<unsigned char> outputbuffer;
			std::shared_mutex MonitorsLock;
			std::vector<Screen_Capture::Monitor> Monitors;

			std::function<void(const std::shared_ptr<Socket::Socket> &socket, const std::vector<Screen_Capture::Monitor> &)> onMonitorsChanged;
			std::function<void(const std::shared_ptr<Socket::Socket> &socket, const RAT::Image &, const Screen_Capture::Monitor &)> onFrameChanged;
			std::function<void(const std::shared_ptr<Socket::Socket> &socket, const RAT::Image &, const Screen_Capture::Monitor &)> onNewFrame;
			std::function<void(const RAT::Image &)> onMouseImageChanged;
			std::function<void(const RAT::Point &)> onMousePositionChanged;
			std::function<void(const std::shared_ptr<Socket::Socket> &, const std::string &)> onClipboardChanged;
			std::function<void(const std::shared_ptr<Socket::Socket>)> onConnection;
			std::function<void(const std::shared_ptr<Socket::Socket> &socket, const Socket::WSMessage)> onMessage;
			std::function<void(const std::shared_ptr<Socket::Socket> &socket, unsigned short code, const std::string)> onDisconnection;
			RAT::ClipboardSharing ShareClip = RAT::ClipboardSharing::NOT_SHARED;
		public:
			RATServer() {}
			virtual ~RATServer() {}
			
			virtual int MaxConnections() const override { return MaxNumConnections; }
			virtual void MaxConnections(int maxconnections) override;
			

			virtual void ShareClipboard(RAT::ClipboardSharing share) override;
			virtual RAT::ClipboardSharing ShareClipboard() const override;
			template <typename STRUCT> void SendStruct_Impl(const std::shared_ptr<Socket::Socket> &socket, STRUCT key, RAT::PACKET_TYPES ptype);
			virtual void SendClipboardChanged(const std::shared_ptr<Socket::Socket> &socket, const std::string &text) override;
			virtual void SendServerSetting(const std::shared_ptr<Socket::Socket> &socket, RAT::ServerSettings serverSetting) override;
			virtual void SendKeyUp(const std::shared_ptr<Socket::Socket> &socket, HSInput::KeyCodes key) override;
			virtual void SendKeyDown(const std::shared_ptr<Socket::Socket> &socket, HSInput::KeyCodes key) override;
			virtual void SendMouseUp(const std::shared_ptr<Socket::Socket> &socket, const HSInput::MouseButtons button) override;
			virtual void SendMouseDown(const std::shared_ptr<Socket::Socket> &socket, const HSInput::MouseButtons button) override;
			virtual void SendMouseScroll(const std::shared_ptr<Socket::Socket> &socket, int offset) override;
			virtual void SendMousePosition(const std::shared_ptr<Socket::Socket> &socket, const RAT::Point &pos) override;
			void Build(const std::shared_ptr<Socket::Socket_Listener> &wsclientconfig);
		};
		class RATServerConfiguration : public IRATServerConfiguration {
		private:
			std::shared_ptr<RATServer> RATServer_;
		public:
			RATServerConfiguration(const std::shared_ptr<RATServer> &c) : RATServer_(c) {}
			virtual ~RATServerConfiguration() {}
			virtual std::shared_ptr<IRATServerConfiguration>
				onMonitorsChanged(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, const std::vector<Screen_Capture::Monitor> &)> &callback) override;
			virtual std::shared_ptr<IRATServerConfiguration>
				onFrameChanged(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, const RAT::Image &, const Screen_Capture::Monitor &)> &callback) override;
			virtual std::shared_ptr<IRATServerConfiguration>
				onNewFrame(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, const RAT::Image &, const Screen_Capture::Monitor &)> &callback) override;
			virtual std::shared_ptr<IRATServerConfiguration> onMouseImageChanged(const std::function<void(const RAT::Image &)> &callback) override;
			virtual std::shared_ptr<IRATServerConfiguration> onMousePositionChanged(const std::function<void(const RAT::Point &)> &callback) override;
			virtual std::shared_ptr<IRATServerConfiguration> onClipboardChanged(const std::function<void(const std::shared_ptr<Socket::Socket> &, const std::string &)> &callback) override;
			virtual std::shared_ptr<IRATServerConfiguration>
				onConnection(const std::function<void(const std::shared_ptr<Socket::Socket>)> &callback) override;
			virtual std::shared_ptr<IRATServerConfiguration>
				onMessage(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, const Socket::WSMessage)> &callback) override;
			virtual std::shared_ptr<IRATServerConfiguration> onDisconnection(
				const std::function<void(const std::shared_ptr<Socket::Socket> &socket, unsigned short code, const std::string)> &callback)
				override;
			virtual std::shared_ptr<IRATServer> Build(const std::shared_ptr<Socket::Socket_Listener> &wslistenerconfig) override;
		};
	}
}