#include "Server.h"
#include <Natupnp.h>
#include <windows.h>
#include <tchar.h>
FILE _iob[] = { *stdin, *stdout, *stderr };
extern "C" { FILE * __cdecl __iob_func(void) { return _iob; } }
static TCHAR szWindowClass[] = _T("RATRAT");
static TCHAR szTitle[] = _T("Now RAT Testing...");
HINSTANCE hInst;
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
class Iabc {

};
class abc final : public Iabc {
public:
	int a;
	int b;
	int c;
};
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	HDC hMemDC, backDC;
	BITMAP bmp;

	static HBITMAP hBit = NULL;
	static HBITMAP tempBitMap;

	static HANDLE hTimer;


	switch (message)
	{
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}


	return 0;
}
int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
)//진입점
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	//wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);//배경 색
	wcex.hbrBackground = NULL;//배경색 설정 안함
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

	if (!RegisterClassEx(&wcex))
	{
		MessageBox(NULL,
			_T("Call to RegisterClassEx failed!"),
			_T("Windows Desktop Guided Tour"),
			NULL);

		return 1;
	}

	hInst = hInstance;

	HWND hWnd = CreateWindow(
		szWindowClass,
		szTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		500, 500,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (!hWnd)
	{
		MessageBox(NULL,
			_T("Call to CreateWindow failed!"),
			_T("Windows Desktop Guided Tour"),
			NULL);

		return 1;
	}
	AllocConsole();
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stderr);
	freopen("CONOUT$", "w", stdout);
	ShowWindow(hWnd,
		nCmdShow);
	HS::RAT_Server::Server server;
	//server.ShareClipboard(HS::RAT::ClipboardSharing::NOT_SHARED);
	//server.ImageCompressionSetting(70);
	//server.FrameChangeInterval(100);
	//server.MouseChangeInterval(50);
	server.MaxConnections(10);
	//server.EncodeImagesAsGrayScale(HS::RAT::ImageEncoding::COLOR);

	server.Run(6001);
	UpdateWindow(hWnd);

	// Main message loop:
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}