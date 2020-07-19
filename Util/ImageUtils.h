#pragma once
#include <windows.h>
#include <memory>
#include <turbojpeg.h>
namespace HS {
	namespace Util {
		HBITMAP BytesToBitmap(unsigned char *data, int width, int height);
		void HBITMAP2BMP(HBITMAP hbit, const char *Path);
	}
}