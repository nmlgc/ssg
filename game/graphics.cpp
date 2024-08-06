/*
 *   Common graphics interface, independent of a specific rendering API
 *
 */

#include "game/graphics.h"
#include "game/input.h"
#include "game/string_format.h"
#include "platform/file.h"
#include "platform/graphics_backend.h"

// Paletted graphics //
// ----------------- //

PALETTE PALETTE::Fade(uint8_t alpha, uint8_t first, uint8_t last) const
{
	PALETTE ret = *this;
	const uint16_t a16 = alpha;
	const auto src_end = (cbegin() + last + 1);
	for(auto src_it = (cbegin() + first); src_it < src_end; src_it++) {
		ret[src_it - cbegin()] = RGBA{
			.r = static_cast<uint8_t>((src_it->r * a16) / 255),
			.g = static_cast<uint8_t>((src_it->g * a16) / 255),
			.b = static_cast<uint8_t>((src_it->b * a16) / 255),
		};
	}
	return ret;
}

void Grp_PaletteSetDefault(void)
{
	if(GrpBackend_PixelFormat().IsChanneled()) {
		return;
	}
	PALETTE palette = {0};
	RGB216::ForEach([&](const RGB216& col) {
		const auto index = col.PaletteIndex();
		palette[index].r = (col.r * (255 / RGB216::MAX));
		palette[index].g = (col.g * (255 / RGB216::MAX));
		palette[index].b = (col.b * (255 / RGB216::MAX));
	});
	GrpBackend_PaletteSet(palette);
}
// ----------------- //

// Screenshots
// -----------

using NUM_TYPE = unsigned int;
static constexpr std::u8string_view EXT = u8".BMP";

static NUM_TYPE ScreenshotNum = 0;
static std::u8string ScreenshotBuf;

void Grp_SetScreenshotPrefix(std::u8string_view prefix)
{
	const auto cap = (prefix.length() + STRING_NUM_CAP<NUM_TYPE> + EXT.size());
	ScreenshotBuf.resize_and_overwrite(cap, [&](auto *p, size_t) {
		return (std::ranges::copy(prefix, p).out - p);
	});
}

// Increments the screenshot number to the next file that doesn't exist yet,
// then opens a write stream for that file.
std::unique_ptr<FILE_STREAM_WRITE> Grp_NextScreenshotStream()
{
	if(ScreenshotBuf.size() == 0) {
		return nullptr;
	}

	// Prevent the theoretical infinite loop...
	while(ScreenshotNum < (std::numeric_limits<NUM_TYPE>::max)()) {
		const auto prefix_len = ScreenshotBuf.size();
		StringCatNum<4>(ScreenshotNum++, ScreenshotBuf);
		ScreenshotBuf += EXT;
		auto ret = FileStreamWrite(ScreenshotBuf.c_str(), true);
		ScreenshotBuf.resize(prefix_len);
		if(ret) {
			return ret;
		}
	}
	return nullptr;
}
// -----------

void Grp_Flip(void)
{
	GrpBackend_Flip((SystemKey_Data & SYSKEY_SNAPSHOT)
		? Grp_NextScreenshotStream()
		: nullptr
	);
}
