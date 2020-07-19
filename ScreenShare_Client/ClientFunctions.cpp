#include "ClientFunctions.h"
#include "Input/InputLite.h"
#include <ScreenCapture.h>

namespace HS {
	namespace RAT_Client {

		int GetNewScreenCaptureRate(const size_t memoryused, size_t maxmemoryused, int imgcapturerateactual, int imgcaptureraterequested)
		{
			if (memoryused >= maxmemoryused) {
				// decrease the capture rate to accomidate slow connections
				return imgcapturerateactual + 100; // slower the capture rate ....
			}
			else if (memoryused < maxmemoryused && imgcapturerateactual != imgcaptureraterequested) {
				// increase the capture rate here
				imgcapturerateactual -= 100; // increase the capture rate ....
				if (imgcapturerateactual <= 0) {
					return imgcaptureraterequested;
				}
				return imgcapturerateactual;
			}
			return imgcapturerateactual;
		}
		int GetNewImageCompression(size_t memoryused, size_t maxmemoryused, int imagecompressionactual, int imagecompressionrequested)
		{
			if (memoryused >= maxmemoryused) {
				if (imagecompressionactual - 10 < 30) { // anything below 30 will be pretty bad....
					return imagecompressionactual;
				}
				return imagecompressionactual - 10;
			}
			else if (memoryused < maxmemoryused && imagecompressionactual != imagecompressionrequested) {
				imagecompressionactual += 10;
				if (imagecompressionactual >= imagecompressionrequested) {
					return imagecompressionrequested;
				}
				return imagecompressionactual;
			}
			return imagecompressionactual;
		}
		void onServerSettingsChanged(std::shared_ptr<Server> &server,
			const std::vector<Screen_Capture::Monitor> &monitors, const RAT::ServerSettings &serversettings)
		{
			server->EncodeImagesAsGrayScale = serversettings.EncodeImagesAsGrayScale;
			server->ImageCompressionSetting = serversettings.ImageCompressionSetting;
			// cap compressionsettings
			if (server->ImageCompressionSetting < 30) {
				server->ImageCompressionSetting = 30;
			}
			else if (server->ImageCompressionSetting > 100) {
				server->ImageCompressionSetting = 100;
			}
			server->ShareClip = serversettings.ShareClip;
			for (auto id : serversettings.MonitorsToWatchId) {
				auto newmonitor = std::find_if(std::begin(monitors), std::end(monitors), [&](const auto &mon) { return mon.Id == id; });
				if (newmonitor != std::end(monitors)) {
					server->MonitorsToWatch.push_back(*newmonitor);
				}
			}
			
		}
		void onGetMonitors(std::shared_ptr<Server> &server, const std::vector<Screen_Capture::Monitor> &monitors)
		{
			
		}

		void onKeyUp(bool ignoreIncomingKeyboardEvents, const std::shared_ptr<Socket::Socket> &socket, const HSInput::KeyCodes &keycode)
		{
			if (!ignoreIncomingKeyboardEvents) {
				HSInput::KeyEvent kevent;
				kevent.Key = keycode;
				kevent.Pressed = false;
				HSInput::SendInput(kevent);
			}
		}

		void onKeyDown(bool ignoreIncomingKeyboardEvents, const std::shared_ptr<Socket::Socket> &socket, const HSInput::KeyCodes &keycode)
		{
			if (!ignoreIncomingKeyboardEvents) {
				HSInput::KeyEvent kevent;
				kevent.Key = keycode;
				kevent.Pressed = true;
				HSInput::SendInput(kevent);
			}
		}

		void onMouseScroll(bool ignoreIncomingMouseEvents, const std::shared_ptr<Socket::Socket> &socket, int offset)
		{
			if (!ignoreIncomingMouseEvents) {
				HSInput::MouseScrollEvent mbevent;
				mbevent.Offset = offset;
				HSInput::SendInput(mbevent);
			}
		}

		void onMouseUp(bool ignoreIncomingMouseEvents, const std::shared_ptr<Socket::Socket> &socket, const HSInput::MouseButtons &button)
		{
			if (!ignoreIncomingMouseEvents) {
				HSInput::MouseButtonEvent mbevent;
				mbevent.Pressed = false;
				mbevent.Button = button;
				HSInput::SendInput(mbevent);
			}
		}
		void onMouseDown(bool ignoreIncomingMouseEvents, const std::shared_ptr<Socket::Socket> &socket, const HSInput::MouseButtons &button)
		{
			if (!ignoreIncomingMouseEvents) {
				HSInput::MouseButtonEvent mbevent;
				mbevent.Pressed = true;
				mbevent.Button = button;
				HSInput::SendInput(mbevent);
			}
		}
		void onMousePosition(bool ignoreIncomingMouseEvents, const std::shared_ptr<Socket::Socket> &socket, const RAT::Point &pos)
		{
			if (!ignoreIncomingMouseEvents) {
				HSInput::MousePositionAbsoluteEvent mbevent;
				mbevent.X = pos.X;
				mbevent.Y = pos.Y;
				HSInput::SendInput(mbevent);
			}
		}
		void onClipboardChanged(bool shareclip, const std::string &str, std::shared_ptr<Clipboard::IClipboard_Manager> clipboard)
		{
			if (shareclip) {
				clipboard->copy(str);
			}
		}
	}
}
