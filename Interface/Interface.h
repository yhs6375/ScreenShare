#pragma once
#include <Windows.h>
#include <thread>
#include <memory>
namespace HS {
	namespace Interface {
		class WindowStruct {
		public:
			HWND hWnd;
			HINSTANCE hInst;
			WNDCLASSEXW wcex;
			HBITMAP hBitmap = NULL;
			std::thread *uithread;
		public:
			WindowStruct();
			~WindowStruct();
			void SetWindowClass();
			void CreateWindowAndShow();
			void DrawScreen(HBITMAP hbitmap, RECT *r, bool redraw);
			static LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
			void threadProc();
		};
		std::shared_ptr<WindowStruct> MakeNewWindow();
	}
}