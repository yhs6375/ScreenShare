#pragma once
#include "Network/Socket.h"
#include "Input/InputLite.h"
#include <ScreenCapture.h>

#include <algorithm>
#include <math.h>
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

namespace HS {
	namespace RAT {
		enum Logging_Levels { Debug_log_level, ERROR_log_level, FATAL_log_level, INFO_log_level, WARN_log_level };
		const std::string Logging_level_Names[] = { "DEBUG", "ERROR", "FATAL", "INFO", "WARN" };
		inline void Log(Logging_Levels level, const char *file, int line, const char *func, std::ostringstream &data)
		{
#if __ANDROID__

			std::ostringstream buffer;
			buffer << Logging_level_Names[level] << ": FILE: " << file << " Line: " << line << " Func: " << func << " Msg: " << data.str();
			auto msg = buffer.str();
			switch (level) {
			case (Debug_log_level):
				__android_log_print(ANDROID_LOG_DEBUG, APPNAME, "%s", msg.c_str());
				break;
			case (ERROR_log_level):
				__android_log_print(ANDROID_LOG_ERROR, APPNAME, "%s", msg.c_str());
				break;
			case (FATAL_log_level):
				__android_log_print(ANDROID_LOG_FATAL, APPNAME, "%s", msg.c_str());
				break;
			case (INFO_log_level):
				__android_log_print(ANDROID_LOG_INFO, APPNAME, "%s", msg.c_str());
				break;
			case (WARN_log_level):
				__android_log_print(ANDROID_LOG_WARN, APPNAME, "%s", msg.c_str());
				break;
			default:
				__android_log_print(ANDROID_LOG_INFO, APPNAME, "%s", msg.c_str());
			}
#else
			std::cout << Logging_level_Names[level] << ": FILE: " << file << " Line: " << line << " Func: " << func << " Msg: " << data.str()
				<< std::endl;

#endif
		}
		const int PixelStride = 4;
		struct Point {
			Point() : X(0), Y(0) {}
			Point(const Point &p) : Point(p.X, p.Y) {}
			Point(int x, int y) : X(x), Y(y) {}
			int X, Y;
		};
		typedef Point Size;
		class Rect {

		public:
			Rect() : Height(0), Width(0) {}
			Rect(const Rect &rect) : Rect(rect.Origin, rect.Height, rect.Width) {}
			Rect(Point origin, int height, int width) : Origin(origin), Height(height), Width(width) {}
			auto Center() const { return Point(Origin.X + (Width / 2), Origin.Y + (Height / 2)); }

			Point Origin;
			int Height, Width;
			auto top() const { return Origin.Y; }
			auto bottom() const { return Origin.Y + Height; }
			void bottom(int b) { Height = b - Origin.Y; }
			auto left() const { return Origin.X; }
			auto right() const { return Origin.X + Width; }
			void right(int r) { Width = r - Origin.X; }
			auto Contains(const Point &p) const
			{
				if (p.X < left())
					return false;
				if (p.Y < top())
					return false;
				if (p.X >= right())
					return false;
				if (p.Y >= bottom())
					return false;
				return true;
			}
			void Expand_To_Include(const Point &p)
			{
				if (p.X <= left())
					Origin.X = p.X;
				if (p.Y <= top())
					Origin.Y = p.Y;
				if (right() <= p.X)
					Width = p.X - left();
				if (bottom() <= p.Y)
					Height = p.Y - top();
			}
		};
		inline auto operator==(const Point &p1, const Point &p2) { return p1.X == p2.X && p1.Y == p2.Y; }
		inline auto operator!=(const Point &p1, const Point &p2) { return !(p1 == p2); }
		inline auto operator==(const Rect &p1, const Rect &p2) { return p1.Origin == p2.Origin && p1.Height == p2.Height && p1.Width == p2.Width; }
		inline auto SquaredDistance(const Point &p1, const Point &p2)
		{
			auto dx = abs(p1.X - p2.X);
			auto dy = abs(p1.Y - p2.Y);
			return dx * dx + dy * dy;
		}
		inline auto SquaredDistance(const Point &p, const Rect &r)
		{
			auto cx = std::max(std::min(p.X, r.right()), r.left());
			auto cy = std::max(std::min(p.Y, r.bottom()), r.top());
			return SquaredDistance(Point(cx, cy), p);
		}
		inline auto Distance(const Point &p1, const Point &p2) { return sqrt(SquaredDistance(p1, p2)); }

		inline auto Distance(const Point &p, const Rect &r) { return Distance(p, r.Center()); }
		struct Image {
			Image() {}
			Image(const Rect &r, const unsigned char *d, const size_t &l) : Rect_(r), Data(d), Length(l) {}
			Rect Rect_;
			const unsigned char *Data = nullptr;
			size_t Length = 0;
		};

		enum Server_Status { SERVER_RUNNING, SERVER_STOPPING, SERVER_STOPPED };
		enum Client_Status { CLIENT_RUNNING, CLIENT_STOPPING, CLIENT_STOPPED };
		enum class ImageEncoding : unsigned char { COLOR, GRAYSCALE };
		enum class ClipboardSharing : unsigned char { NOT_SHARED, SHARED };
		enum class PACKET_TYPES : unsigned int {
			INVALID,
			HTTP_MSG,
			ONMONITORSCHANGED,
			ONFRAMECHANGED,
			ONNEWFRAME,
			ONMOUSEIMAGECHANGED,
			ONMOUSEPOSITIONCHANGED,
			ONKEYUP,
			ONKEYDOWN,
			ONMOUSEUP,
			ONMOUSEDOWN,
			ONMOUSESCROLL,
			ONCLIPBOARDTEXTCHANGED,
			ONSERVERSETTINGSCHANGED,
			// use LAST_PACKET_TYPE as the starting point of your custom packet types. Everything before this is used internally by the library
			LAST_PACKET_TYPE
		};

		struct ServerSettings {
			ClipboardSharing ShareClip = ClipboardSharing::NOT_SHARED;
			int ImageCompressionSetting = 70;
			ImageEncoding EncodeImagesAsGrayScale = ImageEncoding::COLOR;
			std::vector<int> MonitorsToWatchId;
		};
		class IConfig {
		public:
			virtual ~IConfig() {}
			virtual void ShareClipboard(ClipboardSharing share) = 0;
			virtual ClipboardSharing ShareClipboard() const = 0;
		};
	}
}
#define RAT_LOG(level, msg)                                                                                                                       \
    {                                                                                                                                                \
        \
std::ostringstream buffersl134nonesd;                                                                                                                \
        \
buffersl134nonesd                                                                                                                                           \
            << msg;                                                                                                                                  \
        \
HS::RAT::Log(level, __FILE__, __LINE__, __func__, buffersl134nonesd);                                                                           \
    \
}