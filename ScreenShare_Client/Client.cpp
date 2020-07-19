#include "Client.h"
using namespace std::chrono_literals;
namespace HS {
	namespace RAT_Client {
		ClientImpl::ClientImpl() {
			
		}
		ClientImpl::~ClientImpl() {
			ScreenCaptureManager_.reset();
			Clipboard_.reset(); // make sure to prevent race conditions
		}
		void ClientImpl::ScreenCaptureSetting() {
			Clipboard_ = Clipboard::CreateClipboard()
				->onText([&](const std::string &text) {
				if (ShareClip == RAT::ClipboardSharing::SHARED && ratClient) {
					server->socket->send(ratClient->PrepareClipboardChanged(text), Socket::CompressionOptions::NO_COMPRESSION);
				}
			})
				->run();

			ScreenCaptureManager_ =
				Screen_Capture::CreateCaptureConfiguration([&]() {
				AllMonitors = Screen_Capture::GetMonitors();
				if (!ratClient)
					return AllMonitors;
				return AllMonitors;
			})
				->onNewFrame([&](const Screen_Capture::Image &img, const Screen_Capture::Monitor &monitor) {
				if (isGetServerSettings) {
					if (!testtest) {
						testtest = true;
						printf("\nonNewFrame\n");
						if (!ratClient)
							return;
						auto msg = ratClient->PrepareNewFrame(img, monitor, ImageCompressionSettingActual,
							EncodeImagesAsGrayScale == RAT::ImageEncoding::GRAYSCALE ||
							server->EncodeImagesAsGrayScale == RAT::ImageEncoding::GRAYSCALE);
						SendServer(msg);
					}
				}
			})
				->onFrameChanged([&](const Screen_Capture::Image &img, const Screen_Capture::Monitor &monitor) {
				/*printf("\nonFrameChanged\n");
				if (!ratClient)
					return;

				auto newscreencapturerate = GetNewScreenCaptureRate(ratClient->MemoryUsed(), MaxMemoryUsed, ScreenImageCaptureRateActual,
					ScreenImageCaptureRateRequested);
				if (newscreencapturerate != ScreenImageCaptureRateActual) {
					ScreenImageCaptureRateActual = newscreencapturerate;
					ScreenCaptureManager_->setFrameChangeInterval(std::chrono::milliseconds(newscreencapturerate));
				}
				ImageCompressionSettingActual = GetNewImageCompression(ratClient->MemoryUsed(), MaxMemoryUsed,
					ImageCompressionSettingActual, ImageCompressionSettingRequested);

				auto msg = ratClient->PrepareFrameChanged(img, monitor, ImageCompressionSettingActual,
					EncodeImagesAsGrayScale == RAT::ImageEncoding::GRAYSCALE ||
					server->EncodeImagesAsGrayScale == RAT::ImageEncoding::GRAYSCALE);
				SendServer(msg);*/
			})
				->onMouseChanged([&](const Screen_Capture::Image *img, const Screen_Capture::MousePoint &point) {
				/*printf("\nonMouseChanged\n");
				if (!ratClient)
					return;
				if (img) {
					SendServer(ratClient->PrepareMouseImageChanged(*img));
				}
				SendServer(ratClient->PrepareMousePositionChanged(point.Position));*/
			})
				->start_capturing();
			ScreenCaptureManager_->setMouseChangeInterval(std::chrono::milliseconds(MouseCaptureRate));
			ScreenCaptureManager_->setFrameChangeInterval(std::chrono::milliseconds(ScreenImageCaptureRateActual));
			ScreenCaptureManager_->pause();
		}
		void ClientImpl::Run(std::string ip) {
			Status_ = RAT::Client_Status::CLIENT_RUNNING;
			auto connector = Socket::CreateClient();
			ratClient = CreateClientDriverConfiguration()
				->onConnection([&](const std::shared_ptr<Socket::Socket> &socket) {
				printf("connected.\n");
				ScreenCaptureSetting();
				server = std::make_shared<Server>();
				server->socket = socket;
				auto m = Screen_Capture::GetMonitors();
				printf("%d %d %d\n", m.size(), m[0].Width, m[0].Height);
				server->MonitorsToWatch = m;
				SendServer(ratClient->PrepareMonitorsChanged(m));
				ScreenCaptureManager_->resume();
			})
				->onMessage([&](const std::shared_ptr<Socket::Socket> &socket, const Socket::WSMessage &msg) {
				printf("@@@@@@@@@@@@@@@%s\n", msg.data);
			})
				->onDisconnection([&](const std::shared_ptr<Socket::Socket> &socket, unsigned short code, const std::string &msg) {
				ScreenCaptureManager_->pause();
			})
				->onKeyUp([&](const std::shared_ptr<Socket::Socket> &socket, HSInput::KeyCodes key) {
				onKeyUp(IgnoreIncomingKeyboardEvents, socket, key);
			})
				->onKeyDown([&](const std::shared_ptr<Socket::Socket> &socket, HSInput::KeyCodes key) {
				onKeyDown(IgnoreIncomingKeyboardEvents, socket, key);
			})
				->onMouseUp([&](const std::shared_ptr<Socket::Socket> &socket, HSInput::MouseButtons button) {
				onMouseUp(IgnoreIncomingMouseEvents, socket, button);
			})
				->onMouseDown([&](const std::shared_ptr<Socket::Socket> &socket, HSInput::MouseButtons button) {
				onMouseDown(IgnoreIncomingMouseEvents, socket, button);
			})
				->onMouseScroll([&](const std::shared_ptr<Socket::Socket> &socket, int offset) {
				onMouseScroll(IgnoreIncomingMouseEvents, socket, offset);
			})
				->onMousePosition([&](const std::shared_ptr<Socket::Socket> &socket, const RAT::Point &pos) {
				onMousePosition(IgnoreIncomingMouseEvents, socket, pos);
			})
				->onClipboardChanged(
					[&](const std::string &text) { onClipboardChanged(ShareClip == RAT::ClipboardSharing::SHARED, text, Clipboard_); })
				->onServerSettingsChanged(
					[&](const std::shared_ptr<Socket::Socket> &socket, const RAT::ServerSettings &serversettings) {
				onServerSettingsChanged(server, AllMonitors, serversettings);
				isGetServerSettings = true;
			})->Build(connector);
			connector->connect(ip);
		}
		void ClientImpl::SendServer(const Socket::WSMessage &msg, Socket::CompressionOptions compressOptions) {
			if (!server) {
				return;
			}
			server->socket->send(msg, compressOptions);
		}
		Client::Client() { ClientImpl_ = new ClientImpl(); }
		Client::~Client() { delete ClientImpl_; }
		void Client::ShareClipboard(RAT::ClipboardSharing share) { ClientImpl_->ShareClip = share; }
		RAT::ClipboardSharing Client::ShareClipboard() const { return ClientImpl_->ShareClip; }
		
		
		void Client::FrameChangeInterval(int delay_in_ms)
		{
			ClientImpl_->ScreenImageCaptureRateRequested = ClientImpl_->ScreenImageCaptureRateActual = delay_in_ms;
			if (ClientImpl_->ratClient) {
				ClientImpl_->ScreenCaptureManager_->setFrameChangeInterval(std::chrono::milliseconds(delay_in_ms));
			}
		}
		int Client::FrameChangeInterval() const { return ClientImpl_->ScreenImageCaptureRateRequested; }
		void Client::MouseChangeInterval(int delay_in_ms)
		{
			ClientImpl_->MouseCaptureRate = delay_in_ms;
			if (ClientImpl_->ratClient) {
				ClientImpl_->ScreenCaptureManager_->setMouseChangeInterval(std::chrono::milliseconds(delay_in_ms));
			}
		}
		int Client::MouseChangeInterval() const { return ClientImpl_->MouseCaptureRate; }
		void Client::ImageCompressionSetting(int compression)
		{
			ClientImpl_->ImageCompressionSettingRequested = ClientImpl_->ImageCompressionSettingActual = compression;
		}
		int Client::ImageCompressionSetting() const { return ClientImpl_->ImageCompressionSettingRequested; }
		void Client::EncodeImagesAsGrayScale(RAT::ImageEncoding usegrayscale) { ClientImpl_->EncodeImagesAsGrayScale = usegrayscale; }
		RAT::ImageEncoding Client::EncodeImagesAsGrayScale() const { return ClientImpl_->EncodeImagesAsGrayScale; }
		void Client::Run(std::string ip) {
			ClientImpl_->Run(ip);
			while (ClientImpl_->Status_ == RAT::Client_Status::CLIENT_RUNNING) {
				std::this_thread::sleep_for(50ms);
			}
		}
	}
}