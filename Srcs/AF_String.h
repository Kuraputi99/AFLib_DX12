#ifndef AF_STRING_H
#define AF_STRING_H

#include <string>
#include <string_view>

namespace AFLib {
	// UTF8からUTF16に切り替える
	std::wstring UTF8ToUTF16(std::string_view text);

	// コンソールにログを出力する
	void DebugLog(std::string_view msg);
}

#endif // !AF_STRING_H