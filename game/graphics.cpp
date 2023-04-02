/*
 *   Common graphics interface, independent of a specific rendering API
 *
 */

#include "game/graphics.h"
#include <algorithm>

// Paletted graphics //
// ----------------- //

PALETTE PALETTE::Fade(uint8_t alpha, uint8_t first, uint8_t last) const
{
	PALETTE ret = *this;
	const uint16_t a16 = alpha;
	const auto src_end = (cbegin() + last + 1);
	for(auto src_it = (cbegin() + first); src_it < src_end; src_it++) {
		ret[src_it - cbegin()] = RGBA{
			.r = uint8_t( (src_it->r * a16) / 255 ),
			.g = uint8_t( (src_it->g * a16) / 255 ),
			.b = uint8_t( (src_it->b * a16) / 255 ),
		};
	}
	return ret;
}
// ----------------- //
