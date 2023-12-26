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
static std::u8string::iterator BufPastPrefix;

void ScreenshotSetPrefix(std::u8string_view prefix)
{
	Buf.resize((prefix.length() + STRING_NUM_CAP<NUM_TYPE> + EXT.size() + 1));
	BufPastPrefix = std::copy(prefix.begin(), prefix.end(), Buf.begin());
}

std::unique_ptr<FILE_STREAM_WRITE> ScreenshotNextStream()
{
	if(Buf.size() == 0) {
		return nullptr;
	}

	// Prevent the theoretical infinite loop...
	while(Num < (std::numeric_limits<decltype(Num)>::max)()) {
		auto buf_p = StringCopyNum<4>(Num++, BufPastPrefix);
		buf_p = std::copy(EXT.begin(), EXT.end(), buf_p);
		*(buf_p++) = '\0';
		auto ret = FileStreamWrite(Buf.data(), true);
		if(ret) {
			return ret;
		}
	}
	return nullptr;
}
