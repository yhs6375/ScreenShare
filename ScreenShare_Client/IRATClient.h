#pragma once
#include "RAT.h"
namespace HS {
	namespace RAT_Client {
		class IRATClient : public RAT::IConfig {
		public:
			virtual ~IRATClient() {}
			virtual size_t MemoryUsed() const = 0;
			virtual Socket::WSMessage PrepareMonitorsChanged(const std::vector<Screen_Capture::Monitor> &monitors) = 0;
			// imagecompression is [0, 100]    = [WORST, BEST]   Better compression also takes more time .. 70 seems to work well
			virtual Socket::WSMessage PrepareFrameChanged(const Screen_Capture::Image &image, const Screen_Capture::Monitor &monitor,
				int imagecompression = 70, bool usegrayscale = false) = 0;
			// imagecompression is [0, 100]    = [WORST, BEST]
			virtual Socket::WSMessage PrepareNewFrame(const Screen_Capture::Image &image, const Screen_Capture::Monitor &monitor,
				int imagecompression = 70, bool usegrayscale = false) = 0;
			virtual Socket::WSMessage PrepareMouseImageChanged(const Screen_Capture::Image &image) = 0;
			virtual Socket::WSMessage PrepareMousePositionChanged(const HS::Screen_Capture::Point &pos) = 0;
			virtual Socket::WSMessage PrepareClipboardChanged(const std::string &text) = 0;
		};
		class IRATClientConfiguration {
		public:
			virtual ~IRATClientConfiguration() {}
			virtual std::shared_ptr<IRATClientConfiguration>
				onKeyUp(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, HSInput::KeyCodes key)> &callback) = 0;
			virtual std::shared_ptr<IRATClientConfiguration>
				onKeyDown(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, HSInput::KeyCodes key)> &callback) = 0;
			virtual std::shared_ptr<IRATClientConfiguration>
				onMouseUp(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, HSInput::MouseButtons button)> &callback) = 0;
			virtual std::shared_ptr<IRATClientConfiguration>
				onMouseDown(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, HSInput::MouseButtons button)> &callback) = 0;
			virtual std::shared_ptr<IRATClientConfiguration>
				onMouseScroll(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, int offset)> &callback) = 0;
			virtual std::shared_ptr<IRATClientConfiguration>
				onMousePosition(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, const RAT::Point &pos)> &callback) = 0;
			virtual std::shared_ptr<IRATClientConfiguration> onClipboardChanged(const std::function<void(const std::string &text)> &callback) = 0;
			virtual std::shared_ptr<IRATClientConfiguration>
				onConnection(const std::function<void(const std::shared_ptr<Socket::Socket>)> &callback) = 0;
			virtual std::shared_ptr<IRATClientConfiguration>
				onMessage(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, const Socket::WSMessage)> &callback) = 0;
			virtual std::shared_ptr<IRATClientConfiguration> onDisconnection(
				const std::function<void(const std::shared_ptr<Socket::Socket> &socket, unsigned short code, const std::string)> &callback) = 0;
			virtual std::shared_ptr<IRATClientConfiguration> onServerSettingsChanged(
				const std::function<void(const std::shared_ptr<Socket::Socket> &socket, const RAT::ServerSettings &settings)> &callback) = 0;

			virtual std::shared_ptr<IRATClient> Build(const std::shared_ptr<Socket::Socket_Client> &wsclientconfig) = 0;
		};
		std::shared_ptr<IRATClientConfiguration> CreateClientDriverConfiguration();
	}
}