#pragma once
#include "RAT.h"
#include "RATClient.h"
#include <string>
#include <chrono>
#include <thread>
#include <shared_mutex>
#include "ClientFunctions.h"
namespace HS {
	namespace RAT_Client {
		class ClientImpl {
		public:
			RAT::Client_Status Status_ = RAT::Client_Status::CLIENT_STOPPED;
			std::shared_ptr<Screen_Capture::IScreenCaptureManager> ScreenCaptureManager_;
			std::shared_ptr<Clipboard::IClipboard_Manager> Clipboard_;
			std::vector<Screen_Capture::Monitor> AllMonitors;
			RAT::ClipboardSharing ShareClip = RAT::ClipboardSharing::NOT_SHARED;
			std::shared_ptr<IRATClient> ratClient;
			std::shared_ptr<Server> server;

			int ImageCompressionSettingRequested = 70;
			int ImageCompressionSettingActual = 70;
			RAT::ImageEncoding EncodeImagesAsGrayScale = RAT::ImageEncoding::COLOR;
			size_t MaxMemoryUsed = 1024 * 1024 * 100;
			int ScreenImageCaptureRateActual = 100;
			int ScreenImageCaptureRateRequested = 100;
			int MouseCaptureRate = 50;
			bool IgnoreIncomingKeyboardEvents = false;
			bool IgnoreIncomingMouseEvents = false;

			bool isGetServerSettings = false;
			bool testtest = false;
		public:
			ClientImpl();
			~ClientImpl();
			void Run(std::string ip);
			void ScreenCaptureSetting();
			void SendServer(const Socket::WSMessage &msg, Socket::CompressionOptions compressOptions = Socket::CompressionOptions::NO_COMPRESSION);
		};
		class Client {
			ClientImpl *ClientImpl_;
		public:
			Client();
			~Client();
			void ShareClipboard(RAT::ClipboardSharing clipsharing);
			RAT::ClipboardSharing ShareClipboard() const;
			void FrameChangeInterval(int delay_in_ms);
			int FrameChangeInterval() const;
			void MouseChangeInterval(int delay_in_ms);
			int MouseChangeInterval() const;
			// imagecompression is [0, 100]    = [WORST, BEST]
			void ImageCompressionSetting(int compression);
			int ImageCompressionSetting() const;
			void EncodeImagesAsGrayScale(RAT::ImageEncoding encoding);
			RAT::ImageEncoding EncodeImagesAsGrayScale() const;
			void Run(std::string ip);
		};
	}
}