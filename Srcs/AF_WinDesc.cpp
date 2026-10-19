#include "AF_PCH.h"
#include "AF_WinDesc.h"

namespace {
	HWND pHwnd = nullptr;
	HINSTANCE pHinstance = nullptr;
	std::wstring pClassName;
	UINT prevConsoleCP = 0;

	// オリジナルのウィンドウプロシージャを作成
	LRESULT CALLBACK AF_WinProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
		switch (msg) {
		case WM_DESTROY: PostQuitMessage(0); return 0;
		}

		return DefWindowProcW(hWnd, msg, wp, lp);
	}

	// UTF16に変換
	std::wstring UTF8To16(std::string_view text) {
		if (text.empty()) return {};

		const int len = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);

		if (len <= 0) return {};

		std::wstring ws(static_cast<size_t>(len), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), ws.data(), len);

		return ws;
	}
}

// ライブラリを初期化
int InitLib(const WindowDesc& desc) {
	if (pHwnd) return -1;

	/*-コンソールに日本語を出力できるようにする-*/
	prevConsoleCP = GetConsoleOutputCP();
	SetConsoleOutputCP(CP_UTF8);

	pHinstance = desc.hInstance;
	pClassName = UTF8To16(desc.className);

	/*-ウィンドウクラスの作成-*/
	const std::wstring wMenu = UTF8To16(desc.menuName);
	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(wc);
	wc.style = desc.style;
	wc.lpfnWndProc = desc.wndProc ? desc.wndProc : AF_WinProc;
	wc.cbClsExtra = desc.clsExtra;
	wc.cbWndExtra = desc.wndExtra;
	wc.hInstance = desc.hInstance;
	wc.hIcon = desc.icon;
	wc.hCursor = desc.cursor ? desc.cursor : LoadCursorW(nullptr, IDC_ARROW);
	wc.hbrBackground = desc.background;
	wc.lpszMenuName = wMenu.empty() ? nullptr : wMenu.c_str();
	wc.lpszClassName = pClassName.c_str();
	wc.hIconSm = desc.iconSmall;

	if (!RegisterClassExW(&wc)) {
		pClassName.clear();
		return -1;
	}

	/*-描画領域-*/
	RECT rc = { 0, 0, desc.width, desc.height };
	AdjustWindowRectEx(&rc, desc.windowStyle, desc.menuName.empty() ? FALSE : TRUE, desc.exStyle);

	/*-ウィンドウを作成-*/
	const std::wstring wTitle = UTF8To16(desc.title);
	pHwnd = CreateWindowExW(
		desc.exStyle,
		pClassName.c_str(),
		wTitle.c_str(),
		desc.windowStyle,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rc.right - rc.left,
		rc.bottom - rc.top,
		nullptr,
		nullptr,
		pHinstance,
		nullptr
	);

	if (!pHwnd) {
		UnregisterClassW(pClassName.c_str(), pHinstance);
		pClassName.clear();
		return -1;
	}

	/*-ウィンドウを表示-*/
	ShowWindow(pHwnd, SW_SHOW);

	return 0;
}

// 更新されるメッセージを処理
int ProcessMessage() {
	if (!pHwnd) return -1;

	MSG msg = {};
	while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) return -1;
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return 0;
}

// ライブラリを終了
int EndLib() {
	if (pHwnd && IsWindow(pHwnd)) DestroyWindow(pHwnd);

	pHwnd = nullptr;

	if (!pClassName.empty()) {
		UnregisterClassW(pClassName.c_str(), pHinstance);
		pClassName.clear();
	}

	if (prevConsoleCP != 0) {
		SetConsoleOutputCP(prevConsoleCP);
		prevConsoleCP = 0;
	}

	return 0;
}

// ウィンドウハンドルを取得
HWND GetWindowHandle() {
	return pHwnd;
}