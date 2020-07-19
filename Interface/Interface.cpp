#include "Interface.h"
namespace HS {
	namespace Interface {
		WindowStruct::WindowStruct() {
			SetWindowClass();
		}
		WindowStruct::~WindowStruct() {
			uithread->detach();
			delete uithread;
		}
		void WindowStruct::SetWindowClass() {
			HWND hWnd2 = static_cast<HWND>(GetCurrentProcess());
			hInst = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
			
			wcex.cbSize = sizeof(WNDCLASSEX);

			wcex.style = CS_HREDRAW | CS_VREDRAW;
			wcex.lpfnWndProc = WindowStruct::WndProc;
			wcex.cbClsExtra = 0;
			wcex.cbWndExtra = 0;
			wcex.hInstance = hInst;
			wcex.hIcon = 0;
			wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
			wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
			wcex.lpszMenuName = 0;
			wcex.lpszClassName = L"TestRAT";
			wcex.hIconSm = 0;

			//메인 윈도우 클래스 등록
			RegisterClassExW(&wcex);
		}
		void WindowStruct::CreateWindowAndShow() {
			uithread = new std::thread(&WindowStruct::threadProc, this);
		}
		std::shared_ptr<WindowStruct> MakeNewWindow() {
			return std::make_shared<WindowStruct>();
		}
		void WindowStruct::ResizeWindow(int x, int y, int width, int height) {

		}
		void WindowStruct::DrawScreen(HBITMAP _hBitmap, RECT *r, bool redraw) {
			hBitmap = _hBitmap;
			InvalidateRect(hWnd, r, redraw);
		}
		LRESULT CALLBACK WindowStruct::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
			WindowStruct *pThis;
			pThis = reinterpret_cast<WindowStruct*>(GetWindowLongPtr(hWnd, GWL_USERDATA));

			PAINTSTRUCT ps;
			HDC hdc;
			HDC hMemDC, backDC;
			BITMAP bmp;

			static HBITMAP tempBitMap;
			switch (msg)
			{
			case WM_PAINT:
				if (pThis) {
					hdc = BeginPaint(hWnd, &ps);
					if (pThis->hBitmap != NULL) {
						hMemDC = CreateCompatibleDC(hdc);
						backDC = CreateCompatibleDC(hdc);

						tempBitMap = (HBITMAP)SelectObject(backDC, pThis->hBitmap);
						GetObject(pThis->hBitmap, sizeof(BITMAP), &bmp);
						SelectObject(hMemDC, pThis->hBitmap);

						BitBlt(backDC, 0, 0, bmp.bmWidth, bmp.bmHeight, hMemDC, 0, 0, SRCCOPY);
						BitBlt(hdc, 0, 0, bmp.bmWidth, bmp.bmHeight, backDC, 0, 0, SRCCOPY);

						DeleteObject(SelectObject(backDC, tempBitMap));
						DeleteObject(pThis->hBitmap);
						DeleteDC(backDC);
						DeleteDC(hMemDC);
						pThis->hBitmap = NULL;
					}
					EndPaint(hWnd, &ps);
				}
				break;
			case WM_DESTROY:
				PostQuitMessage(0);  // 창닫기 가능
				return 0;
			case WM_LBUTTONDOWN:  // 클릭 시 Beep
				MessageBeep(0);
				return 0;

			}
			return(DefWindowProc(hWnd, msg, wParam, lParam));
		}
		void WindowStruct::threadProc() {
			hWnd = CreateWindowW(L"TestRAT", L"TestRAT", WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT, 0, 470, 370, nullptr, nullptr, hInst, nullptr);
			if (!hWnd)
			{
				return;
			}
			SetWindowLongPtr(hWnd, GWL_USERDATA, (LONG_PTR)this);
			ShowWindow(hWnd, SW_SHOW);
			UpdateWindow(hWnd);

			MSG msg;
			while (GetMessage(&msg, NULL, 0, 0)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
}
