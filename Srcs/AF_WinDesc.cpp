#include "AF_PCH.h"
#include "../Includes/AF_WinDesc.h"

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

	std::wstring UTF8To16(std::string_view text) {
		if (text.empty()) return {};

		const int len = MultiByteToWideChar(
			CP_UTF8,
			0,
			text.data(),
			static_cast<int>(text.size()),
			nullptr,
			0
		);

		if (len <= 0) return {};

		std::wstring ws(static_cast<size_t>(len), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), ws.data(), len);

		return ws;
	}
}

// ライブラリを初期化
int InitLib(const WindowDesc desc) {
	if (pHwnd) return false;

	/*-コンソールに日本語を出力できるようにする-*/
	prevConsoleCP = GetConsoleOutputCP();
	SetConsoleOutputCP(CP_UTF8);

	pHinstance = desc.hInstance;
	pClassName = UTF8To16(desc.className);

	return 0;
}

