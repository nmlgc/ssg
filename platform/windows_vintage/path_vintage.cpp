/*
 *   Common paths for vintage Windows systems
 *
 */

#define WIN32_LEAN_AND_MEAN

#include "platform/path.h"
#include <windows.h>

static std::u8string PathData;

std::u8string_view PathForData(void)
{
	if(!PathData.empty()) {
		return PathData;
	}

	// Like pbg's original code, but updated to Unicode.
	const auto* cmdline_w = (GetCommandLineW() + 1);
	const auto* p = wcschr(cmdline_w, '\"');
	if(!p || !wcschr(cmdline_w, '\\')) {
		return {};
	}
	while(*p != '\\') {
		p--;
	}
	const size_t len_w = ((p + 1) - cmdline_w);
	PathData.resize_and_overwrite((len_w * 4), [&](auto buf, size_t len_utf8) {
		auto* lpstr = reinterpret_cast<LPSTR>(buf);
		return WideCharToMultiByte(
			CP_UTF8, 0, cmdline_w, len_w, lpstr, len_utf8, nullptr, nullptr
		);
	});
	return PathData;
}
