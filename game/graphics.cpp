/*
 *   Common graphics interface, independent of a specific rendering API
 *
 */

#include "game/graphics.h"
#include "DirectXUTYs/DD_UTY.H"
#include <algorithm>
#include <ranges>

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
	if(DxObj.PixelFormat.IsChanneled()) {
		return;
	}
	PALETTE palette = {0};
	for(const auto r : std::views::iota(0, 6)) {
		for(const auto g : std::views::iota(0, 6)) {
			for(const auto b : std::views::iota(0, 6)) {
				const auto col = RGB256(r, g, b);
				palette[col].r = (r * (255 / 5));
				palette[col].g = (g * (255 / 5));
				palette[col].b = (b * (255 / 5));
			}
		}
	}
	GrpSetPalette(palette);
}
// ----------------- //
