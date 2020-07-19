#include "ImageUtils.h"
namespace HS {
	namespace Util {
		HBITMAP BytesToBitmap(unsigned char *data, int width, int height) {
			HDC hdc = GetDC(NULL);
			BITMAPINFOHEADER bmih;
			bmih.biSize = sizeof(BITMAPINFOHEADER);
			bmih.biWidth = width;
			bmih.biHeight = -height;
			bmih.biPlanes = 1;
			bmih.biBitCount = 32;
			bmih.biCompression = BI_RGB;
			bmih.biSizeImage = width * height * 4;
			bmih.biXPelsPerMeter = 0;
			bmih.biYPelsPerMeter = 0;
			bmih.biClrUsed = 0;
			bmih.biClrImportant = 0;

			BITMAPINFO dbmi;
			ZeroMemory(&dbmi, sizeof(dbmi));
			dbmi.bmiHeader = bmih;
			dbmi.bmiColors->rgbBlue = 0;
			dbmi.bmiColors->rgbGreen = 0;
			dbmi.bmiColors->rgbRed = 0;
			dbmi.bmiColors->rgbReserved = 0;
			void* bits = NULL;
			HBITMAP hbit = CreateDIBSection(hdc, &dbmi, DIB_RGB_COLORS, &bits, NULL, 0);
			memcpy(bits, data, width*height*4);
			ReleaseDC(NULL, hdc);
			return hbit;
		}
		void HBITMAP2BMP(HBITMAP hbit, const char *Path){
			BITMAPFILEHEADER fh;
			BITMAPINFOHEADER ih;
			BITMAP bit;
			BITMAPINFO *pih;
			int PalSize;
			HANDLE hFile;
			DWORD dwWritten, Size;
			HDC hdc;

			hdc = GetDC(NULL);
			GetObject(hbit, sizeof(BITMAP), &bit);

			ih.biSize = sizeof(BITMAPINFOHEADER);
			ih.biWidth = bit.bmWidth;
			ih.biHeight = bit.bmHeight;
			ih.biPlanes = 1;
			ih.biBitCount = 32;
			ih.biCompression = BI_RGB;
			ih.biSizeImage = 0;
			ih.biXPelsPerMeter = 0;
			ih.biYPelsPerMeter = 0;
			ih.biClrUsed = 0;
			ih.biClrImportant = 0;

			pih = (BITMAPINFO *)malloc(ih.biSize);
			pih->bmiHeader = ih;
			GetDIBits(hdc, hbit, 0, bit.bmHeight, NULL, pih, DIB_RGB_COLORS);
			ih = pih->bmiHeader;

			if (ih.biSizeImage == 0) {
				ih.biSizeImage = ((((ih.biWidth*ih.biBitCount) + 31) & ~31) >> 3) * ih.biHeight;
			}
			Size = ih.biSize + ih.biSizeImage;
			pih = (BITMAPINFO *)realloc(pih, Size);
			GetDIBits(hdc, hbit, 0, bit.bmHeight, (PBYTE)pih + ih.biSize, pih, DIB_RGB_COLORS);
			fh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
			fh.bfReserved1 = 0;
			fh.bfReserved2 = 0;
			fh.bfSize = Size + sizeof(BITMAPFILEHEADER);
			fh.bfType = 0x4d42;

			hFile = CreateFile(Path, GENERIC_WRITE, 0, NULL,CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			WriteFile(hFile, &fh, sizeof(fh), &dwWritten, NULL);
			WriteFile(hFile, pih, Size, &dwWritten, NULL);

			ReleaseDC(NULL, hdc);

			CloseHandle(hFile);

		}
	}
}
