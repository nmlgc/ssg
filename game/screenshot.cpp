/*
 *   Screenshot filename
 *
 */

#include "game/screenshot.h"
#include "game/string_format.h"

using NUM_TYPE = unsigned int;
static constexpr std::u8string_view EXT = u8".BMP";

static NUM_TYPE Num = 0;
static std::u8string Buf;

void ScreenshotSetPrefix(std::u8string_view prefix)
{
	const auto cap = (prefix.length() + STRING_NUM_CAP<NUM_TYPE> + EXT.size());
	Buf.resize_and_overwrite(cap, [&](decltype(Buf)::value_type* p, size_t) {
		return (std::ranges::copy(prefix, p).out - p);
	});
}

std::unique_ptr<FILE_STREAM_WRITE> ScreenshotNextStream()
{
	if(Buf.size() == 0) {
		return nullptr;
	}

	// Prevent the theoretical infinite loop...
	while(Num < (std::numeric_limits<decltype(Num)>::max)()) {
		const auto prefix_len = Buf.size();
		StringCatNum<4>(Num++, Buf);
		Buf += EXT;
		auto ret = FileStreamWrite(Buf.c_str(), true);
		Buf.resize(prefix_len);
		if(ret) {
			return ret;
		}
	}
	return nullptr;
}
