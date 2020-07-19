#pragma once
#include "Clipboard/Clipboard.h"
#include "RAT.h"
#include <memory>
#include <string>

namespace HS {
	namespace RAT_Client {
		struct Point;

		struct Server {
			RAT::ClipboardSharing ShareClip = RAT::ClipboardSharing::NOT_SHARED;
			int ImageCompressionSetting = 70;

			bool IgnoreIncomingKeyboardEvents = false;
			bool IgnoreIncomingMouseEvents = false;
			RAT::ImageEncoding EncodeImagesAsGrayScale = RAT::ImageEncoding::COLOR;
			std::vector<Screen_Capture::Monitor> MonitorsToWatch;

			std::shared_ptr<Socket::Socket> socket;
		};

		int GetNewScreenCaptureRate(size_t memoryused, size_t maxmemoryused, int imgcapturerateactual, int imgcaptureraterequested);
		int GetNewImageCompression(size_t memoryused, size_t maxmemoryused, int imagecompressionactual, int imagecompressionrequested);

		void onServerSettingsChanged(std::shared_ptr<Server> &server,
			const std::vector<Screen_Capture::Monitor> &monitors, const RAT::ServerSettings &serversettings);
		void onGetMonitors(std::shared_ptr<Server> &server, const std::vector<Screen_Capture::Monitor> &monitors);

		void onKeyUp(bool ignoreIncomingKeyboardEvents, const std::shared_ptr<Socket::Socket> &socket, const HSInput::KeyCodes &keycode);

		void onKeyUp(bool ignoreIncomingKeyboardEvents, const std::shared_ptr<Socket::Socket> &socket, const HSInput::KeyCodes &keycode);
		void onKeyDown(bool ignoreIncomingKeyboardEvents, const std::shared_ptr<Socket::Socket> &socket, const HSInput::KeyCodes &keycode);

		void onMouseUp(bool ignoreIncomingMouseEvents, const std::shared_ptr<Socket::Socket> &socket, const HSInput::MouseButtons &button);
		void onMouseDown(bool ignoreIncomingMouseEvents, const std::shared_ptr<Socket::Socket> &socket, const HSInput::MouseButtons &button);
		void onMouseScroll(bool ignoreIncomingMouseEvents, const std::shared_ptr<Socket::Socket> &socket, int offset);
		void onMousePosition(bool ignoreIncomingMouseEvents, const std::shared_ptr<Socket::Socket> &socket, const RAT::Point &pos);
		void onClipboardChanged(bool shareclipboard, const std::string &str, std::shared_ptr<Clipboard::IClipboard_Manager> clipboard);
	} // namespace RAT_Client
}
