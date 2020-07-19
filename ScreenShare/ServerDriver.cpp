#include "ServerDriver.h"
namespace HS {
	namespace RAT_Server {
		//RATServer 함수 구현
		void RATServer::MaxConnections(int maxconnections)
		{
			assert(maxconnections >= 0);
			MaxNumConnections = maxconnections;
		}
		void RATServer::MouseImageChanged(const std::shared_ptr<Socket::Socket> &socket, const unsigned char *data, size_t len)
		{
			if (!onMouseImageChanged)
				return;
			if (len >= sizeof(RAT::Rect)) {
				RAT::Image img(*reinterpret_cast<const RAT::Rect *>(data), data + sizeof(RAT::Rect), len - sizeof(RAT::Rect));
				if (len >= sizeof(RAT::Rect) + (img.Rect_.Width * img.Rect_.Height * RAT::PixelStride)) {
					return onMouseImageChanged(img);
				}
			}
			socket->close(1000, "Received invalid lenght on onMouseImageChanged");
		}
		void RATServer::MousePositionChanged(const std::shared_ptr<Socket::Socket> &socket, const unsigned char *data, size_t len)
		{
			if (!onMousePositionChanged)
				return;
			if (len == sizeof(HS::Screen_Capture::Point)) {
				return onMousePositionChanged(*reinterpret_cast<const RAT::Point *>(data));
			}
			socket->close(1000, "Received invalid lenght on onMousePositionChanged");
		}

		void RATServer::MonitorsChanged(const std::shared_ptr<Socket::Socket> &socket, const unsigned char *data, size_t len)
		{
			if (!onMonitorsChanged)
				return;
			auto num = len / sizeof(Screen_Capture::Monitor);
			Monitors.clear();
			if (len == num * sizeof(Screen_Capture::Monitor) && num < 8) {
				for (decltype(num) i = 0; i < num; i++) {
					Monitors.push_back(*(Screen_Capture::Monitor *)data);
					data += sizeof(Screen_Capture::Monitor);
				}
				return onMonitorsChanged(socket, Monitors);
			}
			else if (len == 0) {
				// it is possible to have no monitors.. shouldnt disconnect in that case
				return onMonitorsChanged(socket, Monitors);
			}
			socket->close(1000, "Invalid Monitor Count");
		}
		void RATServer::ClipboardTextChanged(const std::shared_ptr<Socket::Socket> &socket, const unsigned char *data, size_t len)
		{
			if (ShareClip == RAT::ClipboardSharing::NOT_SHARED || !onClipboardChanged)
				return;
			if (len < 1024 * 100) { // 100K max
				std::string str(reinterpret_cast<const char *>(data), len);
				onClipboardChanged(socket, str);
			}
		}
		template <typename CALLBACKTYPE> void RATServer::Frame(const std::shared_ptr<Socket::Socket> &socket, const unsigned char *data, size_t len, CALLBACKTYPE &&cb)
		{
			int monitor_id = 0;
			if (len < sizeof(RAT::Rect) + sizeof(monitor_id)) {
				return socket->close(1000, "Invalid length on onFrameChanged");
			}

			auto jpegDecompressor = tjInitDecompress();
			int jpegSubsamp(0), outwidth(0), outheight(0);

			auto src = (unsigned char *)data;
			memcpy(&monitor_id, src, sizeof(monitor_id));
			src += sizeof(monitor_id);
			RAT::Rect rect;
			memcpy(&rect, src, sizeof(rect));
			src += sizeof(rect);

			auto monitor = std::find_if(begin(Monitors), end(Monitors), [monitor_id](const auto &m) { return m.Id == monitor_id; });
			if (monitor == end(Monitors)) {
				RAT_LOG(RAT::Logging_Levels::INFO_log_level, "Monitor Id doesnt exist!");
				return;
			}
			len -= sizeof(RAT::Rect) + sizeof(monitor_id);

			if (tjDecompressHeader2(jpegDecompressor, src, static_cast<unsigned long>(len), &outwidth, &outheight, &jpegSubsamp) == -1) {
				RAT_LOG(RAT::Logging_Levels::ERROR_log_level, tjGetErrorStr());
			}
			std::lock_guard<std::mutex> lock(outputbufferLock);
			outputbuffer.reserve(outwidth * outheight * RAT::PixelStride);

			if (tjDecompress2(jpegDecompressor, src, static_cast<unsigned long>(len), (unsigned char *)outputbuffer.data(), outwidth, 0, outheight,
				TJPF_RGBX, 2048 | TJFLAG_NOREALLOC) == -1) {
				RAT_LOG(RAT::Logging_Levels::ERROR_log_level, tjGetErrorStr());
			}
			RAT::Image img(rect, outputbuffer.data(), outwidth * outheight * RAT::PixelStride);

			assert(outwidth == img.Rect_.Width && outheight == img.Rect_.Height);
			cb(img, *monitor);
			tjDestroy(jpegDecompressor);
		}
		void RATServer::Build(const std::shared_ptr<Socket::Socket_Listener> &wsclientconfig)
		{
			ClientCount = 0;
			wsclientconfig
				->onConnection([&](const std::shared_ptr<Socket::Socket> &socket, const Socket::HttpHeader &header) {
				if (MaxNumConnections > 0 && ClientCount + 1 > MaxNumConnections) {
					socket->close(1000, "Closing due to max number of connections!");
				}
				RAT_LOG(RAT::Logging_Levels::INFO_log_level, "onConnection ");
				if (onConnection)
					onConnection(socket);
				ClientCount += 1;
			})
				->onDisconnection([&](const std::shared_ptr<Socket::Socket> &socket, unsigned short code, const std::string &msg) {
				RAT_LOG(RAT::Logging_Levels::INFO_log_level, "onDisconnection ");
				ClientCount -= 1;
				if (onDisconnection)
					onDisconnection(socket, code, msg);
			})
				->onMessage([&](const std::shared_ptr<Socket::Socket> &socket, const Socket::WSMessage &message) {
				auto p = RAT::PACKET_TYPES::INVALID;
				assert(message.len >= sizeof(p));

				p = *reinterpret_cast<const RAT::PACKET_TYPES *>(message.data);
				const auto datastart = message.data + sizeof(p);
				size_t datasize = message.len - sizeof(p);
				switch (p) {
				case RAT::PACKET_TYPES::ONMONITORSCHANGED:
					MonitorsChanged(socket, datastart, datasize);
					break;
				case RAT::PACKET_TYPES::ONFRAMECHANGED:
					if (onFrameChanged) {
						Frame(socket, datastart, datasize, [&](const auto &img, const auto &monitor) { onFrameChanged(socket, img, monitor); });
					}
					break;
				case RAT::PACKET_TYPES::ONNEWFRAME:
					if (onNewFrame) {
						Frame(socket, datastart, datasize, [&](const auto &img, const auto &monitor) { onNewFrame(socket, img, monitor); });
					}
					break;
				case RAT::PACKET_TYPES::ONMOUSEIMAGECHANGED:
					MouseImageChanged(socket, datastart, datasize);
					break;
				case RAT::PACKET_TYPES::ONMOUSEPOSITIONCHANGED:
					MousePositionChanged(socket, datastart, datasize);
					break;
				case RAT::PACKET_TYPES::ONCLIPBOARDTEXTCHANGED:
					ClipboardTextChanged(socket, datastart, datasize);
					break;
				default:
					if (onMessage)
						onMessage(socket, message); // pass up the chain
					break;
				}

			});
		}

		void RATServer::ShareClipboard(RAT::ClipboardSharing share) { ShareClip = share; }
		RAT::ClipboardSharing RATServer::ShareClipboard() const { return ShareClip; }
		template <typename STRUCT> void RATServer::SendStruct_Impl(const std::shared_ptr<Socket::Socket> &socket, STRUCT key, RAT::PACKET_TYPES ptype)
		{
			if (!socket) {
				RAT_LOG(RAT::Logging_Levels::INFO_log_level, "SendKey called on a socket that is not open yet");
				return;
			}
			const auto size = sizeof(ptype) + sizeof(key);
			auto ptr(std::shared_ptr<unsigned char>(new unsigned char[size], [](auto *p) { delete[] p; }));
			*reinterpret_cast<RAT::PACKET_TYPES *>(ptr.get()) = ptype;
			memcpy(ptr.get() + sizeof(ptype), &key, sizeof(key));

			Socket::WSMessage buf;
			buf.code = Socket::OpCode::BINARY;
			buf.Buffer = ptr;
			buf.len = size;
			buf.data = ptr.get();
			socket->send(buf, Socket::CompressionOptions::NO_COMPRESSION);
		}
		void RATServer::SendClipboardChanged(const std::shared_ptr<Socket::Socket> &socket, const std::string &text)
		{
			if (!socket) {
				RAT_LOG(RAT::Logging_Levels::INFO_log_level, "SendClipboardText called on a socket that is not open yet");
				return;
			}
			if (socket->is_loopback())
				return; // dont send clipboard info to ourselfs as it will cause a loop

			auto ptype = RAT::PACKET_TYPES::ONCLIPBOARDTEXTCHANGED;
			auto size = sizeof(ptype) + text.size();
			auto ptr(std::shared_ptr<unsigned char>(new unsigned char[size], [](auto *p) { delete[] p; }));
			*reinterpret_cast<RAT::PACKET_TYPES *>(ptr.get()) = ptype;
			memcpy(ptr.get() + sizeof(ptype), text.data(), text.size());

			Socket::WSMessage buf;
			buf.code = Socket::OpCode::BINARY;
			buf.Buffer = ptr;
			buf.len = size;
			buf.data = ptr.get();
			socket->send(buf, Socket::CompressionOptions::NO_COMPRESSION);
		}
		void RATServer::SendServerSetting(const std::shared_ptr<Socket::Socket> &socket, RAT::ServerSettings serverSetting) {
			if (!socket) {
				RAT_LOG(RAT::Logging_Levels::INFO_log_level, "SendServerSetting called on a socket that is not open yet");
				return;
			}
			size_t test_size = sizeof(serverSetting.MonitorsToWatchId);
			size_t test_size2 = sizeof(RAT::ServerSettings);
			size_t test_size3 = sizeof(serverSetting);
			/*if (socket->is_loopback())
				return;*/
			auto ptype = RAT::PACKET_TYPES::ONSERVERSETTINGSCHANGED;
			auto sizeWithoutIds = sizeof(serverSetting.ShareClip) + sizeof(serverSetting.ImageCompressionSetting) + sizeof(serverSetting.EncodeImagesAsGrayScale);
			auto size = sizeof(ptype) + sizeWithoutIds + serverSetting.MonitorsToWatchId.size()*sizeof(int);
			auto ptr(std::shared_ptr<unsigned char>(new unsigned char[size], [](auto *p) {delete[] p; }));
			*reinterpret_cast<RAT::PACKET_TYPES *>(ptr.get()) = ptype;
			auto start = ptr.get() + sizeof(ptype);
			memcpy(start, &serverSetting.ShareClip, sizeof(serverSetting.ShareClip));
			start += sizeof(serverSetting.ShareClip);
			memcpy(start, &serverSetting.ImageCompressionSetting, sizeof(serverSetting.ImageCompressionSetting));
			start += sizeof(serverSetting.ImageCompressionSetting);
			memcpy(start, &serverSetting.EncodeImagesAsGrayScale, sizeof(serverSetting.EncodeImagesAsGrayScale));
			start += sizeof(serverSetting.EncodeImagesAsGrayScale);
			for (auto id : serverSetting.MonitorsToWatchId) {
				memcpy(start, &id, sizeof(int));
				start += sizeof(int);
			}
			Socket::WSMessage buf;
			buf.code = Socket::OpCode::BINARY;
			buf.Buffer = ptr;
			buf.len = size;
			buf.data = ptr.get();
			socket->send(buf, Socket::CompressionOptions::NO_COMPRESSION);
		}
		void RATServer::SendKeyUp(const std::shared_ptr<Socket::Socket> &socket, HSInput::KeyCodes key) { SendStruct_Impl(socket, key, RAT::PACKET_TYPES::ONKEYUP); }
		void RATServer::SendKeyDown(const std::shared_ptr<Socket::Socket> &socket, HSInput::KeyCodes key) { SendStruct_Impl(socket, key, RAT::PACKET_TYPES::ONKEYDOWN); }
		void RATServer::SendMouseUp(const std::shared_ptr<Socket::Socket> &socket, const HSInput::MouseButtons button) { SendStruct_Impl(socket, button, RAT::PACKET_TYPES::ONMOUSEUP); }
		void RATServer::SendMouseDown(const std::shared_ptr<Socket::Socket> &socket, const HSInput::MouseButtons button) { SendStruct_Impl(socket, button, RAT::PACKET_TYPES::ONMOUSEDOWN); }
		void RATServer::SendMouseScroll(const std::shared_ptr<Socket::Socket> &socket, int offset) { SendStruct_Impl(socket, offset, RAT::PACKET_TYPES::ONMOUSESCROLL); }
		void RATServer::SendMousePosition(const std::shared_ptr<Socket::Socket> &socket, const RAT::Point &pos) { SendStruct_Impl(socket, pos, RAT::PACKET_TYPES::ONMOUSEPOSITIONCHANGED); }


		//ServerDriverConfiguration 함수 구현
		std::shared_ptr<IRATServerConfiguration>
			RATServerConfiguration::onMonitorsChanged(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, const std::vector<Screen_Capture::Monitor> &)> &callback)
		{
			assert(!RATServer_->onMonitorsChanged);
			RATServer_->onMonitorsChanged = callback;
			return std::make_shared<RATServerConfiguration>(RATServer_);
		}
		std::shared_ptr<IRATServerConfiguration>
			RATServerConfiguration::onFrameChanged(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, const RAT::Image &, const Screen_Capture::Monitor &)> &callback)
		{
			assert(!RATServer_->onFrameChanged);
			RATServer_->onFrameChanged = callback;
			return std::make_shared<RATServerConfiguration>(RATServer_);
		}
		std::shared_ptr<IRATServerConfiguration>
			RATServerConfiguration::onNewFrame(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, const RAT::Image &, const Screen_Capture::Monitor &)> &callback)
		{
			assert(!RATServer_->onNewFrame);
			RATServer_->onNewFrame = callback;
			return std::make_shared<RATServerConfiguration>(RATServer_);
		}
		std::shared_ptr<IRATServerConfiguration> RATServerConfiguration::onMouseImageChanged(const std::function<void(const RAT::Image &)> &callback)
		{
			assert(!RATServer_->onMouseImageChanged);
			RATServer_->onMouseImageChanged = callback;
			return std::make_shared<RATServerConfiguration>(RATServer_);
		}
		std::shared_ptr<IRATServerConfiguration> RATServerConfiguration::onMousePositionChanged(const std::function<void(const RAT::Point &)> &callback)
		{
			assert(!RATServer_->onMousePositionChanged);
			RATServer_->onMousePositionChanged = callback;
			return std::make_shared<RATServerConfiguration>(RATServer_);
		}
		std::shared_ptr<IRATServerConfiguration> RATServerConfiguration::onClipboardChanged(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, const std::string &)> &callback)
		{
			assert(!RATServer_->onClipboardChanged);
			RATServer_->onClipboardChanged = callback;
			return std::make_shared<RATServerConfiguration>(RATServer_);
		}

		std::shared_ptr<IRATServerConfiguration>
			RATServerConfiguration::onConnection(const std::function<void(const std::shared_ptr<Socket::Socket>)> &callback)
		{
			assert(!RATServer_->onConnection);
			RATServer_->onConnection = callback;
			return std::make_shared<RATServerConfiguration>(RATServer_);
		}
		std::shared_ptr<IRATServerConfiguration>
			RATServerConfiguration::onMessage(const std::function<void(const std::shared_ptr<Socket::Socket> &socket, const Socket::WSMessage)> &callback)
		{
			assert(!RATServer_->onMessage);
			RATServer_->onMessage = callback;
			return std::make_shared<RATServerConfiguration>(RATServer_);
		}
		std::shared_ptr<IRATServerConfiguration> RATServerConfiguration::onDisconnection(
			const std::function<void(const std::shared_ptr<Socket::Socket> &socket, unsigned short code, const std::string)> &callback)
		{
			assert(!RATServer_->onDisconnection);
			RATServer_->onDisconnection = callback;
			return std::make_shared<RATServerConfiguration>(RATServer_);
		}
		std::shared_ptr<IRATServer> RATServerConfiguration::Build(const std::shared_ptr<Socket::Socket_Listener> &wslistenerconfig)
		{
			RATServer_->Build(wslistenerconfig);
			return RATServer_;
		}
	}
}