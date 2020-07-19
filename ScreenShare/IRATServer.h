#pragma once
#include "Input/InputLite.h"
#include <ScreenCapture.h>
#include "RAT.h"
namespace HS {
	namespace RAT_Server {
		class IRATServer : public RAT::IConfig {
		public:
			virtual ~IRATServer() {}
			virtual void MaxConnections(int maxconnections) = 0;
			virtual int MaxConnections() const = 0;
			
			virtual void SendKeyUp(const std::shared_ptr<Socket::Socket> &socket, HSInput::KeyCodes key) = 0;
			virtual void SendKeyDown(const std::shared_ptr<Socket::Socket> &socket, HSInput::KeyCodes key) = 0;
			virtual void SendMouseUp(const std::shared_ptr<Socket::Socket> &socket, const HSInput::MouseButtons button) = 0;
			virtual void SendMouseDown(const std::shared_ptr<Socket::Socket> &socket, const HSInput::MouseButtons button) = 0;
			virtual void SendMouseScroll(const std::shared_ptr<Socket::Socket> &socket, int offset) = 0;
			virtual void SendMousePosition(const std::shared_ptr<Socket::Socket> &socket, const RAT::Point &pos) = 0;
			virtual void SendClipboardChanged(const std::shared_ptr<Socket::Socket> &socket, const std::string &text) = 0;
			virtual void SendServerSetting(const std::shared_ptr<Socket::Socket> &socket, RAT::ServerSettings serverSetting) = 0;
		};
		class IRATServerConfiguration{
		public:
			virtual ~IRATServerConfiguration() {}
			// events raised from the server
			virtual std::shared_ptr<IRATServerConfiguration>
				onMonitorsChanged(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, const std::vector<Screen_Capture::Monitor> &)> &callback) = 0;
			virtual std::shared_ptr<IRATServerConfiguration>
				onFrameChanged(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, const RAT::Image &, const HS::Screen_Capture::Monitor &)> &callback) = 0;
			virtual std::shared_ptr<IRATServerConfiguration>
				onNewFrame(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, const RAT::Image &, const HS::Screen_Capture::Monitor &)> &callback) = 0;
			virtual std::shared_ptr<IRATServerConfiguration> onMouseImageChanged(const std::function<void(const RAT::Image &)> &callback) = 0;
			virtual std::shared_ptr<IRATServerConfiguration> onMousePositionChanged(const std::function<void(const RAT::Point &)> &callback) = 0;
			virtual std::shared_ptr<IRATServerConfiguration> onConnection(const std::function<void(const std::shared_ptr<Socket::Socket>)> &callback) = 0;
			virtual std::shared_ptr<IRATServerConfiguration>
				onMessage(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, const Socket::WSMessage)> &callback) = 0;
			virtual std::shared_ptr<IRATServerConfiguration> onDisconnection(
				const std::function<void(const std::shared_ptr<Socket::Socket> &socket, unsigned short code, const std::string)> &callback) = 0;
			virtual std::shared_ptr<IRATServerConfiguration> onClipboardChanged(const std::function<void(const std::shared_ptr<Socket::Socket> &, const std::string &)> &callback) = 0;
			virtual std::shared_ptr<IRATServer> Build(const std::shared_ptr<Socket::Socket_Listener> &wslistenerconfig) = 0;
		};
		std::shared_ptr<IRATServerConfiguration> CreateServerDriverConfiguration();
	}
}