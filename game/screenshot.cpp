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
static constexpr std::u8string_view EXT = u8".BMP";

static NUM_TYPE Num = 0;
static std::u8string Buf;
static size_t PrefixLen;

void ScreenshotSetPrefix(std::u8string_view prefix)
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
		buf_p = std::copy_n(num_str.data(), num_len, buf_p);
		buf_p = std::copy(EXT.begin(), EXT.end(), buf_p);
		*(buf_p++) = '\0';
		auto ret = FileStreamWrite(Buf.data(), true);
		if(ret) {
			return ret;
		}
	}
	return nullptr;
}
