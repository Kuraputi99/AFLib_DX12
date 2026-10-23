#ifndef AF_DRAW_H
#define AF_DRAW_H

namespace AFLib {
	// 色を作る(r, g, b はそれぞれ 0〜255)
	constexpr unsigned int GetColor(int r, int g, int b) {
		return (static_cast<unsigned int>(r & 0xFF) << 16)
			| (static_cast<unsigned int>(g & 0xFF) << 8)
			| static_cast<unsigned int>(b & 0xFF);
	}

	// 塗りつぶした四角形を描く(x2, y2 の位置は含まない)
	int DrawBox(int x1, int y1, int x2, int y2, unsigned int color);
}

#endif // !AF_DRAW_H