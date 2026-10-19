#ifndef AF_WIN_DESC_H
#define AF_WIN_DESC_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN

#ifndef NOMINMAX
#define NOMINMAX
#endif // !NOMINMAX

#include <Windows.h>
#include <string>

// ウィンドウクラスの登録設定
struct WindowDesc {
	UINT style            = CS_HREDRAW | CS_VREDRAW;
	WNDPROC wndProc       = DefWindowProcW;
	int clsExtra          = 0;
	int wndExtra          = 0;
	HINSTANCE hInstance   = GetModuleHandleW(nullptr);
	HICON icon            = nullptr;
	HCURSOR cursor        = LoadCursorW(nullptr, IDC_ARROW);
	HBRUSH background     = nullptr;
	std::string menuName  = "test";
	std::string className = "AFLib_Class";
	HICON iconSmall       = nullptr;

	std::string title	  = "AFLib Window";
	int width			  = 1280;
	int height			  = 720;
	DWORD windowStyle     = WS_OVERLAPPEDWINDOW;
	DWORD exStyle		  = 0;
};

// ライブラリを初期化
int InitLib(const WindowDesc& desc);

// 更新されるメッセージを処理
int ProcessMessage();

// ライブラリを終了
int EndLib();

// ウィンドウハンドルを取得
HWND GetWindowHandle();

#endif // !AF_WIN_DESC_H
