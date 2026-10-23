#ifndef AF_INPUT_H
#define AF_INPUT_H

namespace AFLib {
	// キーが入力されている間に発火
	bool IsKeyPress(int key);

	// キーが押された瞬間に発火
	bool IsKeyDown(int key);

	// キーが離された瞬間に発火
	bool IsKeyUp(int key);

	// マウスカーソルの位置を取得
	void GetMousePosition(int* x, int* y);
}

#endif // !AF_INPUT_H
