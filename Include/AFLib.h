#ifndef AFLIB_H
#define AFLIB_H

namespace AFLib {
	struct WindowDesc {
		UINT style = CS_HREDRAW | CS_VREDRAW;
		WNDPROC wndProc = DefWindowProcW;
		int clsExtra = 0;
		int wndExtra = 0;
		HINSTANCE hInstance = GetModuleHandleW(nullptr);
		HICON icon = nullptr;
		HCURSOR cursor = LoadCursorW(nullptr, IDC_ARROW);
		HBRUSH background = nullptr;
		std:::string ;
	};

	int GetVersion();
}

#endif // !AFLIB_H
