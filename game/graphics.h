/*
 *   Common graphics interface, independent of a specific rendering API
 *
 */

#pragma once

#include <array>
#include <assert.h>

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

// Maps (6 * 6 * 6) = 216 RGB colors to standard palette indices.
constexpr uint8_t RGB256(uint8_t r, uint8_t g, uint8_t b) {
	assert(r < 6);
	assert(g < 6);
	assert(b < 6);
	return (20 + r + (g * 6) + (b * 36));
}

// Sets a default palette covering 216 equally distributed RGB colors. Useful
// for showing some text before loading any of the game's images that would
// otherwise define a palette.
void Grp_PaletteSetDefault(void);
// ----------------- //
