#include "AF_PCH.h"
#include "AF_Input.h"
#include "AF_InputInternal.h"
#include "AF_Application.h"

namespace {
	BYTE currentKeys[256] = {};   // 今のフレームのキー状態
	BYTE previousKeys[256] = {};   // 1つ前のフレームのキー状態

	// 指定したキーが押されているか(最上位ビットが 1 なら押されている)
	bool IsDown(const BYTE* keys, int key) {
		if (key < 0 || key > 255) return false;
		return (keys[key] & 0x80) != 0;
	}
}

namespace AFLib {
	void UpdateInput() {
		std::memcpy(previousKeys, currentKeys, sizeof(currentKeys));

		/*-ウィンドウが選ばれていないときは、何も押されていない扱いにする-*/
		if (GetForegroundWindow() != GetWindowHandle()) {
			std::memset(currentKeys, 0, sizeof(currentKeys));
			return;
		}

		if (!GetKeyboardState(currentKeys)) {
			std::memset(currentKeys, 0, sizeof(currentKeys));
		}
	}

	bool IsKeyDown(int key) {
		return IsDown(currentKeys, key);
	}

	bool IsKeyPressed(int key) {
		return IsDown(currentKeys, key) && !IsDown(previousKeys, key);
	}

	bool IsKeyReleased(int key) {
		return !IsDown(currentKeys, key) && IsDown(previousKeys, key);
	}

	void GetMousePosition(int* x, int* y) {
		POINT pos = {};
		GetCursorPos(&pos);                        // 画面全体での位置
		ScreenToClient(GetWindowHandle(), &pos);   // ウィンドウの描画領域での位置に変換

		if (x) *x = pos.x;
		if (y) *y = pos.y;
	}
}