#include "RATClient.h"

namespace HS {
	namespace RAT_Client{
		void RATClient::KeyUp(const std::shared_ptr<Socket::Socket> &socket, const unsigned char *data, size_t len)
		{
			if (!onKeyUp)
				return;
			if (len == sizeof(HSInput::KeyCodes)) {
				onKeyUp(socket, *reinterpret_cast<const HSInput::KeyCodes *>(data));
			}
			else {
				return socket->close(1000, "Received invalid onKeyUp Event");
			}
		}
		void RATClient::KeyDown(const std::shared_ptr<Socket::Socket> &socket, const unsigned char *data, size_t len)
		{
			if (!onKeyDown)
				return;
			if (len == sizeof(HSInput::KeyCodes)) {
				onKeyDown(socket, *reinterpret_cast<const HSInput::KeyCodes *>(data));
			}
			else {
				return socket->close(1000, "Received invalid onKeyDown Event");
			}
		}
		void RATClient::MouseUp(const std::shared_ptr<Socket::Socket> &socket, const unsigned char *data, size_t len)
		{
			if (!onMouseUp)
				return;
			if (len == sizeof(HSInput::MouseButtons)) {
				return onMouseUp(socket, *reinterpret_cast<const HSInput::MouseButtons *>(data));
			}
			socket->close(1000, "Received invalid onMouseUp Event");
		}

		void RATClient::MouseDown(const std::shared_ptr<Socket::Socket> &socket, const unsigned char *data, size_t len)
		{
			if (!onMouseDown)
				return;
			if (len == sizeof(HSInput::MouseButtons)) {
				return onMouseDown(socket, *reinterpret_cast<const HSInput::MouseButtons *>(data));
			}
			socket->close(1000, "Received invalid onMouseDown Event");
		}
		void RATClient::MousePosition(const std::shared_ptr<Socket::Socket> &socket, const unsigned char *data, size_t len)
		{
			if (!onMousePosition)
				return;
			if (len == sizeof(Screen_Capture::Point)) {
				auto p = *reinterpret_cast<const RAT::Point *>(data);
				return onMousePosition(socket, p);
			}
			socket->close(1000, "Received invalid onMouseDown Event");
		}
		void RATClient::MouseScroll(const std::shared_ptr<Socket::Socket> &socket, const unsigned char *data, size_t len)
		{
			if (!onMouseScroll)
				return;
			if (len == sizeof(int)) {
				return onMouseScroll(socket, *reinterpret_cast<const int *>(data));
			}
			socket->close(1000, "Received invalid onMouseScroll Event");
		}
		//서버가보낸 세팅data bytes를 구조체로 변환
		void RATClient::ServerSettingsChanged(const std::shared_ptr<Socket::Socket> &socket, const unsigned char *data, size_t len)
		{
			if (!onServerSettingsChanged)
				return;
			RAT::ServerSettings serverSetting;
			auto beginsize = sizeof(serverSetting.ShareClip) + sizeof(serverSetting.ImageCompressionSetting) + sizeof(serverSetting.EncodeImagesAsGrayScale);
			auto remainingdata = len - beginsize;
			if (len >= beginsize &&   // min size received at least 1 monitor
				(remainingdata % sizeof(int) == 0)) { // the remaining bytes are divisible by sizeof int
				auto start = data;
				// need to do a manualy copy because of byte packing
				memcpy(&serverSetting.ShareClip, start, sizeof(serverSetting.ShareClip));
				start += sizeof(serverSetting.ShareClip);
				memcpy(&serverSetting.ImageCompressionSetting, start, sizeof(serverSetting.ImageCompressionSetting));
				start += sizeof(serverSetting.ImageCompressionSetting);
				memcpy(&serverSetting.EncodeImagesAsGrayScale, start, sizeof(serverSetting.EncodeImagesAsGrayScale));
				start += sizeof(serverSetting.EncodeImagesAsGrayScale);
				auto begin = reinterpret_cast<const int *>(start);
				auto end = reinterpret_cast<const int *>(start + remainingdata);
				for (auto b = begin; b < end; b++) {
					auto found = std::find_if(std::begin(Monitors), std::end(Monitors), [b](auto &mon) { return mon.Id == *b; });
					if (found != std::end(Monitors)) {
						serverSetting.MonitorsToWatchId.push_back((*found).Id);
					}
				}
				return onServerSettingsChanged(socket, serverSetting);
			}
			socket->close(1000, "Received invalid onClientSettingsChanged Event");
		}
		void RATClient::ClipboardChanged(const unsigned char *data, size_t len)
		{
			if (!onClipboardChanged || ShareClip == RAT::ClipboardSharing::NOT_SHARED)
				return;
			if (len < 1024 * 100) { // 100K max
				std::string str(reinterpret_cast<const char *>(data), len);
				return onClipboardChanged(str);
			}
		}
		void RATClient::Build(const std::shared_ptr<Socket::Socket_Client> &wslistenerconfig)
		{
			
			wslistenerconfig
				->onConnection([&](const std::shared_ptr<Socket::Socket> &socket, const Socket::HttpHeader &header) {
				Socket_ = socket;
				if (onConnection)
					onConnection(socket);
			})
				->onDisconnection([&](const std::shared_ptr<Socket::Socket> &socket, unsigned short code, const std::string &msg) {
				RAT_LOG(RAT::Logging_Levels::INFO_log_level, "onDisconnection  ");
				Socket_.reset();
				if (onDisconnection)
					onDisconnection(socket, code, msg);
			})
				->onMessage([&](const std::shared_ptr<Socket::Socket> &socket, const Socket::WSMessage &message) {

				auto p = RAT::PACKET_TYPES::INVALID;
				assert(message.len >= sizeof(p));

				p = *reinterpret_cast<const RAT::PACKET_TYPES *>(message.data);
				auto datastart = message.data + sizeof(p);
				auto datasize = message.len - sizeof(p);

				switch (p) {
				case RAT::PACKET_TYPES::ONKEYDOWN:
					KeyDown(socket, datastart, datasize);
					break;
				case RAT::PACKET_TYPES::ONKEYUP:
					KeyUp(socket, datastart, datasize);
					break;
				case RAT::PACKET_TYPES::ONMOUSEUP:
					MouseUp(socket, datastart, datasize);
					break;
				case RAT::PACKET_TYPES::ONMOUSEDOWN:
					MouseDown(socket, datastart, datasize);
					break;
				case RAT::PACKET_TYPES::ONMOUSEPOSITIONCHANGED:
					MousePosition(socket, datastart, datasize);
					break;
				case RAT::PACKET_TYPES::ONSERVERSETTINGSCHANGED:
					ServerSettingsChanged(socket, datastart, datasize);
					break;
				case RAT::PACKET_TYPES::ONMOUSESCROLL:
					MouseScroll(socket, datastart, datasize);
					break;
				case RAT::PACKET_TYPES::ONCLIPBOARDTEXTCHANGED:
					ClipboardChanged(datastart, datasize);
					break;
				default:
					if (onMessage)
						onMessage(socket, message);
					break;
				}
			});
		}
		int RowStride2(const Screen_Capture::Image &img) {
			return sizeof(Screen_Capture::ImageBGRA) * Screen_Capture::Width(img);
		}
		Socket::WSMessage RATClient::PrepareImage(const Screen_Capture::Image &img, int imagecompression, bool usegrayscale, const Screen_Capture::Monitor &monitor, RAT::PACKET_TYPES p)
		{
			RAT::Rect r(RAT::Point(img.Bounds.left, img.Bounds.top), Height(img), Width(img));
			auto set = usegrayscale ? TJSAMP_GRAY : TJSAMP_420;
			unsigned long maxsize =
				tjBufSize(Screen_Capture::Width(img), Screen_Capture::Height(img), set) + sizeof(r) + sizeof(p) + sizeof(monitor.Id);
			auto jpegCompressor = tjInitCompress();
			MemoryInUse += maxsize;
			auto buffer = std::shared_ptr<unsigned char>(new unsigned char[maxsize], [maxsize, this](auto *p) {
				delete[] p;
				MemoryInUse -= maxsize;
			});
			auto dst = (unsigned char *)buffer.get();
			memcpy(dst, &p, sizeof(p));
			dst += sizeof(p);
			memcpy(dst, &monitor.Id, sizeof(monitor.Id));
			dst += sizeof(monitor.Id);
			memcpy(dst, &r, sizeof(r));
			dst += sizeof(r);

			auto srcbuffer = std::make_unique<unsigned char[]>(RowStride2(img) * Height(img));
			Screen_Capture::Extract(img, srcbuffer.get(), RowStride2(img) * Height(img));
			auto srcbuf = (unsigned char *)srcbuffer.get();
			auto colorencoding = TJPF_BGRX;
			auto outjpegsize = maxsize;

			if (tjCompress2(jpegCompressor, srcbuf, r.Width, 0, r.Height, colorencoding, &dst, &outjpegsize, set, imagecompression,
				TJFLAG_FASTDCT | TJFLAG_NOREALLOC) == -1) {
				RAT_LOG(RAT::Logging_Levels::ERROR_log_level, tjGetErrorStr());
			}
			//	std::cout << "Sending " << r << std::endl;
			auto finalsize = sizeof(p) + sizeof(r) + sizeof(monitor.Id) + outjpegsize; // adjust the correct size
			tjDestroy(jpegCompressor);
			return Socket::WSMessage{ buffer.get(), finalsize, Socket::OpCode::BINARY, buffer };
		}
		Socket::WSMessage RATClient::PrepareMonitorsChanged(const std::vector<Screen_Capture::Monitor> &monitors)
		{
			{
				std::unique_lock<std::shared_mutex> lock(MonitorsLock);
				Monitors = monitors;
			}
			auto p = static_cast<unsigned int>(RAT::PACKET_TYPES::ONMONITORSCHANGED);
			const auto finalsize = (monitors.size() * sizeof(Screen_Capture::Monitor)) + sizeof(p);

			MemoryInUse += finalsize;
			auto buffer = std::shared_ptr<unsigned char>(new unsigned char[finalsize], [finalsize, this](auto *p) {
				delete[] p;
				MemoryInUse -= finalsize;
			});

			auto buf = buffer.get();
			memcpy(buf, &p, sizeof(p));
			buf += sizeof(p);
			for (auto &a : monitors) {
				memcpy(buf, &a, sizeof(a));
				buf += sizeof(Screen_Capture::Monitor);
			}
			return Socket::WSMessage{ buffer.get(), finalsize, Socket::OpCode::BINARY, buffer };
		}
		Socket::WSMessage RATClient::PrepareFrameChanged(const Screen_Capture::Image &image, const Screen_Capture::Monitor &monitor, int imagecompression, bool usegrayscale)
		{
			assert(imagecompression > 0 && imagecompression <= 100);
			return PrepareImage(image, imagecompression, usegrayscale, monitor, RAT::PACKET_TYPES::ONFRAMECHANGED);
		}
		Socket::WSMessage RATClient::PrepareNewFrame(const Screen_Capture::Image &image, const Screen_Capture::Monitor &monitor, int imagecompression, bool usegrayscale)
		{
			assert(imagecompression > 0 && imagecompression <= 100);
			return PrepareImage(image, imagecompression, usegrayscale, monitor, RAT::PACKET_TYPES::ONNEWFRAME);
		}
		Socket::WSMessage RATClient::PrepareMouseImageChanged(const Screen_Capture::Image &image)
		{
			RAT::Rect r(RAT::Point(0, 0), Height(image), Width(image));
			auto p = static_cast<unsigned int>(RAT::PACKET_TYPES::ONMOUSEIMAGECHANGED);
			auto finalsize = (RowStride2(image) * Screen_Capture::Height(image)) + sizeof(p) + sizeof(r);
			MemoryInUse += finalsize;
			auto buffer = std::shared_ptr<unsigned char>(new unsigned char[finalsize], [finalsize, this](auto *p) {
				delete[] p;
				MemoryInUse -= finalsize;
			});
			auto dst = buffer.get();
			memcpy(dst, &p, sizeof(p));
			dst += sizeof(p);
			memcpy(dst, &r, sizeof(r));
			dst += sizeof(r);
			Screen_Capture::Extract(image, (unsigned char *)dst, RowStride2(image) * Screen_Capture::Height(image));
			return Socket::WSMessage{ buffer.get(), finalsize, Socket::OpCode::BINARY, buffer };
		}
		Socket::WSMessage RATClient::PrepareMousePositionChanged(const Screen_Capture::Point &pos)
		{
			auto p = static_cast<unsigned int>(RAT::PACKET_TYPES::ONMOUSEPOSITIONCHANGED);
			const auto finalsize = sizeof(pos) + sizeof(p);
			MemoryInUse += finalsize;
			auto buffer = std::shared_ptr<unsigned char>(new unsigned char[finalsize], [finalsize, this](auto *p) {
				delete[] p;
				MemoryInUse -= finalsize;
			});
			memcpy(buffer.get(), &p, sizeof(p));
			memcpy(buffer.get() + sizeof(p), &pos, sizeof(pos));
			return Socket::WSMessage{ buffer.get(), finalsize, Socket::OpCode::BINARY, buffer };
		}
		Socket::WSMessage RATClient::PrepareClipboardChanged(const std::string &text)
		{
			auto p = static_cast<unsigned int>(RAT::PACKET_TYPES::ONCLIPBOARDTEXTCHANGED);
			auto finalsize = text.size() + sizeof(p);
			MemoryInUse += finalsize;
			auto buffer = std::shared_ptr<unsigned char>(new unsigned char[finalsize], [finalsize, this](auto *p) {
				delete[] p;
				MemoryInUse -= finalsize;
			});
			memcpy(buffer.get(), &p, sizeof(p));
			memcpy(buffer.get() + sizeof(p), text.data(), text.size());
			return Socket::WSMessage{ buffer.get(), finalsize, Socket::OpCode::BINARY, buffer };
		}


		
		//RATClientConfiguration 함수 구현
		std::shared_ptr<IRATClientConfiguration> RATClientConfiguration::onKeyUp(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, HSInput::KeyCodes key)> &callback)
		{
			assert(!RATClient_->onKeyUp);
			RATClient_->onKeyUp = callback;
			return std::make_shared<RATClientConfiguration>(RATClient_);
		}
		std::shared_ptr<IRATClientConfiguration> RATClientConfiguration::onKeyDown(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, HSInput::KeyCodes key)> &callback)
		{
			assert(!RATClient_->onKeyDown);
			RATClient_->onKeyDown = callback;
			return std::make_shared<RATClientConfiguration>(RATClient_);
		}
		std::shared_ptr<IRATClientConfiguration> RATClientConfiguration::onMouseUp(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, HSInput::MouseButtons button)> &callback)
		{
			assert(!RATClient_->onMouseUp);
			RATClient_->onMouseUp = callback;
			return std::make_shared<RATClientConfiguration>(RATClient_);
		}
		std::shared_ptr<IRATClientConfiguration> RATClientConfiguration::onMouseDown(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, HSInput::MouseButtons button)> &callback)
		{
			assert(!RATClient_->onMouseDown);
			RATClient_->onMouseDown = callback;
			return std::make_shared<RATClientConfiguration>(RATClient_);
		}
		std::shared_ptr<IRATClientConfiguration> RATClientConfiguration::onMouseScroll(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, int offset)> &callback)
		{
			assert(!RATClient_->onMouseScroll);
			RATClient_->onMouseScroll = callback;
			return std::make_shared<RATClientConfiguration>(RATClient_);
		}
		std::shared_ptr<IRATClientConfiguration> RATClientConfiguration::onMousePosition(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, const RAT::Point &pos)> &callback)
		{
			assert(!RATClient_->onMousePosition);
			RATClient_->onMousePosition = callback;
			return std::make_shared<RATClientConfiguration>(RATClient_);
		}
		std::shared_ptr<IRATClientConfiguration> RATClientConfiguration::onClipboardChanged(const std::function<void(const std::string &text)> &callback)
		{
			assert(!RATClient_->onClipboardChanged);
			RATClient_->onClipboardChanged = callback;
			return std::make_shared<RATClientConfiguration>(RATClient_);
		}
		std::shared_ptr<IRATClientConfiguration> RATClientConfiguration::onConnection(const std::function<void(const std::shared_ptr<Socket::Socket>)> &callback)
		{
			assert(!RATClient_->onConnection);
			RATClient_->onConnection = callback;
			return std::make_shared<RATClientConfiguration>(RATClient_);
		}
		std::shared_ptr<IRATClientConfiguration> RATClientConfiguration::onMessage(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, const Socket::WSMessage)> &callback)
		{
			assert(!RATClient_->onMessage);
			RATClient_->onMessage = callback;
			return std::make_shared<RATClientConfiguration>(RATClient_);
		}
		std::shared_ptr<IRATClientConfiguration> RATClientConfiguration::onDisconnection(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, unsigned short code, const std::string)> &callback)
		{
			assert(!RATClient_->onDisconnection);
			RATClient_->onDisconnection = callback;
			return std::make_shared<RATClientConfiguration>(RATClient_);
		}
		std::shared_ptr<IRATClientConfiguration> RATClientConfiguration::onServerSettingsChanged(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, const RAT::ServerSettings &settings)> &callback)
		{
			assert(!RATClient_->onServerSettingsChanged);
			RATClient_->onServerSettingsChanged = callback;
			return std::make_shared<RATClientConfiguration>(RATClient_);
		}
		std::shared_ptr<IRATClient> RATClientConfiguration::Build(const std::shared_ptr<Socket::Socket_Client> &wsclientconfig)
		{
			RATClient_->Build(wsclientconfig);
			return RATClient_;
		}

		std::shared_ptr<IRATClientConfiguration> CreateClientDriverConfiguration()
		{
			return std::make_shared<RATClientConfiguration>(std::make_shared<RATClient>());
		}

	}
}