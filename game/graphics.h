/*
 *   Common graphics interface, independent of a specific rendering API
 *
 */

#pragma once

#include <array>

// Paletted graphics //
// ----------------- //

// RGBA color structure, identical to Win32 GDI's PALETTEENTRY.
struct RGBA {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};
static_assert(sizeof(RGBA) == 4);

struct PALETTE : public std::array<RGBA, 256> {
	// Builds a new palette with the given fade [alpha] value applied onto the
	// given inclusive (!) range of colors. Returns the rest of the palette
	// unchanged.
	PALETTE Fade(uint8_t alpha, uint8_t first = 0, uint8_t last = 255) const;
};
// ----------------- //
