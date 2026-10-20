#ifndef AF_APPLICATION_H
#define AF_APPLICATION_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN

#ifndef NOMINMAX
#define NOMINMAX
#endif // !NOMINMAX

#include <Windows.h>
#include <string>

namespace AFLib {
	// ウィンドウクラスの登録設定
	struct WindowDesc {
		UINT style            = CS_HREDRAW | CS_VREDRAW;                                 // ウィンドウサイズ変更時の描画スタイル
		WNDPROC wndProc       = nullptr;                                                 // ウィンドウプロシージャ
		int clsExtra          = 0;                                                       // ウィンドウクラスごとの追加メモリ
		int wndExtra          = 0;                                                       // ウィンドウごとの追加メモリ
		HINSTANCE hInstance   = GetModuleHandleW(nullptr);                               // インスタンスハンドル
		HICON icon            = nullptr;                                                 // ウィンドウのアイコン
		HCURSOR cursor        = nullptr;                                                 // ウィンドウ上のカーソル
		HBRUSH background     = nullptr;                                                 // ウィンドウの背景色
		std::string menuName;                                                            // ウィンドウのメニュー名
		std::string className = "AFLib_Class";                                           // ウィンドウのクラス名
		HICON iconSmall       = nullptr;                                                 // タスクバーなどに表示される小さなアイコン
		std::string title     = "AFLib Window";                                          // ウィンドウの名前
		int width             = 1280;                                                    // ウィンドウの幅
		int height            = 720;                                                     // ウィンドウの高さ
		DWORD windowStyle     = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX); // ウィンドウの描画スタイル
		DWORD exStyle         = 0;                                                       // ウィンドウの拡張スタイル
	};

	// ライブラリを初期化
	int InitLib(const WindowDesc& desc = {});

	// 更新されるメッセージを処理
	int ProcessMessage();

	// ライブラリを終了
	int EndLib();

	// ウィンドウハンドルを取得
	HWND GetWindowHandle();
}

#endif // !AF_APPLICATION_H
