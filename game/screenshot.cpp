/*
 *   Screenshot filename
 *
 */

#include "game/screenshot.h"
#include <array>

// Returns the number of decimal digits required to store the highest value of
// the given type.
template <typename T> constexpr unsigned int DecimalDigitsBound() {
	return (((241 * sizeof(T)) / 100) + 1);
}

using NUM_TYPE = unsigned int;
static constexpr unsigned int DIGITS_MAX = DecimalDigitsBound<NUM_TYPE>();
static constexpr PATH_STRING_VIEW EXT = _PATH(".BMP");

static NUM_TYPE Num = 0;
static PATH_STRING Buf;
static size_t PrefixLen;

void ScreenshotSetPrefix(PATH_STRING_VIEW prefix)
{
	Buf.resize((prefix.length() + DIGITS_MAX + EXT.size() + 1));
	std::copy(prefix.begin(), prefix.end(), Buf.begin());
	PrefixLen = prefix.length();
}

std::unique_ptr<FILE_STREAM_WRITE> ScreenshotNextStream()
{
	if(Buf.size() == 0) {
		return nullptr;
	}

	// Prevent the theoretical infinite loop...
	while(Num < (std::numeric_limits<decltype(Num)>::max)()) {
		std::array<char, (DIGITS_MAX + 1)> num_str;
		const auto num_len = sprintf(num_str.data(), "%04u", Num++);
		auto buf_p = (Buf.begin() + PrefixLen);

		// Stretch [num_str] to UTF-16 if necessary.
		// std::copy() doesn't handle differing types.
		for(auto i = decltype(num_len){0}; i < num_len; i++) {
			*(buf_p++) = num_str[i];
		}

		buf_p = std::copy(EXT.begin(), EXT.end(), buf_p);
		*(buf_p++) = '\0';
		auto ret = FileStreamWrite(Buf.data(), true);
		if(ret) {
			return ret;
		}
	}
	return nullptr;
}
