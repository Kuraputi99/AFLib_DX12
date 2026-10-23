#include "AF_PCH.h"
#include "AF_Application.h"
#include "AF_String.h"
#include "AF_Graphics.h"
#include "AF_InputInternal.h"

namespace {
	HWND pHwnd              = nullptr;           // ウィンドウハンドル
	HINSTANCE pHinstance    = nullptr;           // インスタンスハンドル
	std::wstring pClassName;                     // ウィンドウクラスの名前
	UINT prevConsoleCP      = 0;                 // コンソールの出力形態
	std::unique_ptr<AFLib::Graphics> graphics;   // 描画担当

	// オリジナルのウィンドウプロシージャを作成
	LRESULT CALLBACK AF_WinProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
		switch (msg) {
		case WM_DESTROY: PostQuitMessage(0); return 0;
		}

		return DefWindowProcW(hWnd, msg, wp, lp);
	}
}

namespace AFLib {
	// ライブラリを初期化
	int InitLib(const WindowDesc& desc) {
		if (pHwnd) {
			DebugLog("ライブラリは既に初期化済みです！");
			return -1;
		}

		/*-コンソールに日本語を出力できるようにする-*/
		prevConsoleCP = GetConsoleOutputCP();
		SetConsoleOutputCP(CP_UTF8);

		/*-変数を代入しておく-*/
		pHinstance = desc.hInstance;
		pClassName = UTF8ToUTF16(desc.className);

		/*-ウィンドウクラスの作成-*/
		const std::wstring wMenu = UTF8ToUTF16(desc.menuName);
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

		/*-ウィンドウクラスの登録チェック-*/
		if (!RegisterClassExW(&wc)) {
			DebugLog("ウィンドウクラスの登録に失敗しました");
			pClassName.clear();
			return -1;
		}

		/*-クライアント領域の計算-*/
		RECT rc = { 0, 0, desc.width, desc.height };
		AdjustWindowRectEx(&rc, desc.windowStyle, desc.menuName.empty() ? FALSE : TRUE, desc.exStyle);

		/*-ウィンドウを作成-*/
		const std::wstring wTitle = UTF8ToUTF16(desc.title);
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

		/*-ウィンドウの作成チェック-*/
		if (!pHwnd) {
			DebugLog("ウィンドウの作成に失敗しました");
			UnregisterClassW(pClassName.c_str(), pHinstance);
			pClassName.clear();
			return -1;
		}

		/*-ウィンドウを表示-*/
		ShowWindow(pHwnd, SW_SHOW);

		/*-DirectX 12 の初期化-*/
		graphics = std::make_unique<Graphics>();
		if (graphics->Init(pHwnd, desc.width, desc.height) != 0) {
			EndLib();
			return -1;
		}

		return 0;
	}

	// 更新されるメッセージを処理
	int ProcessMessage() {
		if (!pHwnd || !graphics) return -1;

		/*-前のフレームを画面に表示-*/
		graphics->DrawScreen();

		/*-メッセージ処理をさばく-*/
		MSG msg = {};
		while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) return -1;
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}

		/*-入力状態を更新-*/
		UpdateInput();

		/*-次のフレームを開始(画面のクリア)-*/
		graphics->ClearScreen();

		return 0;
	}

	// ライブラリを終了
	int EndLib() {
		graphics.reset();

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

	// 描画担当を取得(ライブラリ内部用)
	Graphics * GetGraphics() {
		return graphics.get();
	}
}