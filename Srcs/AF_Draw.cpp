#include "AF_PCH.h"
#include "AF_Draw.h"
#include "AF_Graphics.h"

namespace AFLib {
	int DrawBox(int x1, int y1, int x2, int y2, unsigned int color) {
		Graphics* g = GetGraphics();
		if (!g) return -1;   // InitLib 前は描けない
		return g->DrawBox(x1, y1, x2, y2, color);
	}
}