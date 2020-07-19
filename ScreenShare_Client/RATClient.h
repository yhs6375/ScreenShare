#pragma once
#include <mutex>
#include <shared_mutex>

#include "IRATClient.h"
#include <ScreenCapture.h>
#include <internal/SCCommon.h>
#include <turbojpeg.h>
#include "Input/InputLite.h"
#include <assert.h>

namespace HS {
	namespace RAT_Client{
		class RATClient : public IRATClient {
		private:
			RAT::Point LastMousePosition_;
			std::shared_ptr<Socket::Socket> Socket_;

			
			std::vector<Screen_Capture::Monitor> Monitors;
			std::shared_mutex MonitorsLock;
		
		public:
			std::function<void(const std::shared_ptr<Socket::Socket> &socket, HSInput::KeyCodes key)> onKeyUp;
			std::function<void(const std::shared_ptr<Socket::Socket> &socket, HSInput::KeyCodes key)> onKeyDown;
			std::function<void(const std::shared_ptr<Socket::Socket> &socket, HSInput::MouseButtons button)> onMouseUp;
			std::function<void(const std::shared_ptr<Socket::Socket> &socket, HSInput::MouseButtons button)> onMouseDown;
			std::function<void(const std::shared_ptr<Socket::Socket> &socket, const RAT::ServerSettings &settings)> onServerSettingsChanged;
			std::function<void(const std::shared_ptr<Socket::Socket> &socket, int offset)> onMouseScroll;
			std::function<void(const std::shared_ptr<Socket::Socket> &socket, const RAT::Point &pos)> onMousePosition;
			std::function<void(const std::string &text)> onClipboardChanged;

			std::function<void(const std::shared_ptr<Socket::Socket>)> onConnection;
			std::function<void(const std::shared_ptr<Socket::Socket> &socket, const Socket::WSMessage)> onMessage;
			std::function<void(const std::shared_ptr<Socket::Socket> &socket, unsigned short code, const std::string)> onDisconnection;

			std::atomic<size_t> MemoryInUse;
		public:
			RATClient() { MemoryInUse = 0; }
			virtual ~RATClient() {}
			RAT::ClipboardSharing ShareClip = RAT::ClipboardSharing::NOT_SHARED;

			virtual size_t MemoryUsed() const override { return MemoryInUse; }

			void Build(const std::shared_ptr<Socket::Socket_Client> &wsclientconfig);
			
			virtual void ShareClipboard(RAT::ClipboardSharing share) override { ShareClip = share; }
			virtual RAT::ClipboardSharing ShareClipboard() const override { return ShareClip; }
			
			void KeyUp(const std::shared_ptr<Socket::Socket> &socket, const unsigned char *data, size_t len);
			void KeyDown(const std::shared_ptr<Socket::Socket> &socket, const unsigned char *data, size_t len);
			void MouseUp(const std::shared_ptr<Socket::Socket> &socket, const unsigned char *data, size_t len);
			void MouseDown(const std::shared_ptr<Socket::Socket> &socket, const unsigned char *data, size_t len);
			void MousePosition(const std::shared_ptr<Socket::Socket> &socket, const unsigned char *data, size_t len);
			void MouseScroll(const std::shared_ptr<Socket::Socket> &socket, const unsigned char *data, size_t len);
			void ServerSettingsChanged(const std::shared_ptr<Socket::Socket> &socket, const unsigned char *data, size_t len);
			void ClipboardChanged(const unsigned char *data, size_t len);
			void Build(const std::shared_ptr<Socket::Socket_Listener> &wslistenerconfig);
			Socket::WSMessage PrepareImage(const Screen_Capture::Image &img, int imagecompression, bool usegrayscale, const Screen_Capture::Monitor &monitor, RAT::PACKET_TYPES p);
			virtual Socket::WSMessage PrepareMonitorsChanged(const std::vector<Screen_Capture::Monitor> &monitors) override;
			virtual Socket::WSMessage PrepareFrameChanged(const Screen_Capture::Image &image, const Screen_Capture::Monitor &monitor, int imagecompression, bool usegrayscale) override;
			virtual Socket::WSMessage PrepareNewFrame(const Screen_Capture::Image &image, const Screen_Capture::Monitor &monitor, int imagecompression, bool usegrayscale) override;
			virtual Socket::WSMessage PrepareMouseImageChanged(const Screen_Capture::Image &image) override;
			virtual Socket::WSMessage PrepareMousePositionChanged(const Screen_Capture::Point &pos) override;
			virtual Socket::WSMessage PrepareClipboardChanged(const std::string &text) override;

		};
		class RATClientConfiguration : public IRATClientConfiguration {
			std::shared_ptr<RATClient> RATClient_;

		public:
			RATClientConfiguration(const std::shared_ptr<RATClient> &c) : RATClient_(c) {}
			virtual ~RATClientConfiguration() {}
			

			virtual std::shared_ptr<IRATClientConfiguration>
				onKeyUp(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, HSInput::KeyCodes key)> &callback) override;
			virtual std::shared_ptr<IRATClientConfiguration>
				onKeyDown(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, HSInput::KeyCodes key)> &callback) override;
			virtual std::shared_ptr<IRATClientConfiguration>
				onMouseUp(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, HSInput::MouseButtons button)> &callback) override;
			virtual std::shared_ptr<IRATClientConfiguration>
				onMouseDown(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, HSInput::MouseButtons button)> &callback) override;
			virtual std::shared_ptr<IRATClientConfiguration>
				onMouseScroll(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, int offset)> &callback) override;
			virtual std::shared_ptr<IRATClientConfiguration>
				onMousePosition(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, const RAT::Point &pos)> &callback) override;
			virtual std::shared_ptr<IRATClientConfiguration> onClipboardChanged(const std::function<void(const std::string &text)> &callback) override;
			virtual std::shared_ptr<IRATClientConfiguration>
				onConnection(const std::function<void(const std::shared_ptr<Socket::Socket>)> &callback) override;
			virtual std::shared_ptr<IRATClientConfiguration>
				onMessage(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, const Socket::WSMessage)> &callback) override;
			virtual std::shared_ptr<IRATClientConfiguration> onDisconnection(
				const std::function<void(const std::shared_ptr<Socket::Socket> &socket, unsigned short code, const std::string)> &callback) override;
			virtual std::shared_ptr<IRATClientConfiguration> onServerSettingsChanged(
				const std::function<void(const std::shared_ptr<Socket::Socket> &socket, const RAT::ServerSettings &settings)> &callback) override;
			virtual std::shared_ptr<IRATClient> Build(const std::shared_ptr<Socket::Socket_Client> &wsclientconfig) override;
		};
		std::shared_ptr<IRATClientConfiguration> CreateClientDriverConfiguration();
	}
}