#include "AF_PCH.h"
#include "AF_String.h"

namespace AFLib {
	// UTF8からUTF16に切り替える
	std::wstring UTF8ToUTF16(std::string_view text) {
		if (text.empty()) return {};

		const int len = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
		if (len <= 0) return {};

		std::wstring ws(static_cast<size_t>(len), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), ws.data(), len);

		return ws;
	}

	// コンソールにログを出力する
	void DebugLog(std::string_view msg) {
		std::string text = "[AFLib] ";
		text += msg;
		text += '\n';

		std::fputs(text.c_str(), stderr);
		OutputDebugStringW(UTF8ToUTF16(text).c_str());
	}
}