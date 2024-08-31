/*
 *   Common graphics interface, independent of a specific rendering API
 *
 */

#pragma once

import std.compat;
#include "game/pixelformat.h"
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

struct RGB {
	uint8_t r;
	uint8_t g;
	uint8_t b;

	constexpr RGBA WithAlpha(uint8_t a) const {
		return RGBA{ .r = r, .g = g, .b = b, .a = a };
	}
};
static_assert(sizeof(RGB) == 3);

struct PALETTE : public std::array<RGBA, 256> {
	// Builds a new palette with the given fade [alpha] value applied onto the
	// given inclusive (!) range of colors. Returns the rest of the palette
	// unchanged.
	PALETTE Fade(uint8_t alpha, uint8_t first = 0, uint8_t last = 255) const;
};

// (6 * 6 * 6) = 216 standard colors, available in both channeled and
// palettized modes.
struct RGB216 {
	static constexpr uint8_t MAX = 5;

	const uint8_t r = 0;
	const uint8_t g = 0;
	const uint8_t b = 0;

	consteval RGB216() = default;
	consteval RGB216(uint8_t&& r, uint8_t&& g, uint8_t&& b) : r(r), g(g), b(b) {
		if((r > MAX) || (g > MAX) || (b > MAX)) {
			throw "216-color component out of range";
		}
	}

	RGB216(
		std::unsigned_integral auto r,
		std::unsigned_integral auto g,
		std::unsigned_integral auto b
	) :
		r(r), g(g), b(b)
	{
		assert(r <= MAX);
		assert(g <= MAX);
		assert(b <= MAX);
	}

	static RGB216 Clamped(uint8_t r, uint8_t g, uint8_t b) {
		return RGB216{
			(std::min)(r, MAX), (std::min)(g, MAX), (std::min)(b, MAX)
		};
	}

	constexpr uint8_t PaletteIndex(void) const {
		return (20 + r + (g * (MAX + 1)) + (b * ((MAX + 1) * (MAX + 1))));
	}

	constexpr RGB ToRGB(void) const {
		return RGB{ .r = (r * 50u), .g = (g * 50u), .b = (b * 50u) };
	}

	static void ForEach(std::invocable<const RGB216&> auto&& func) {
		constexpr uint8_t start = 0;
		constexpr uint8_t end = (MAX + 1);
		for(const auto r : std::views::iota(start, end)) {
			for(const auto g : std::views::iota(start, end)) {
				for(const auto b : std::views::iota(start, end)) {
					func(RGB216{ r, g, b });
				}
			}
		}
	}
};

// Sets a default palette covering 216 equally distributed RGB colors. Useful
// for showing some text before loading any of the game's images that would
// otherwise define a palette.
void Grp_PaletteSetDefault(void);
// ----------------- //

// Required to enable the screenshot feature as a whole.
void Grp_SetScreenshotPrefix(std::u8string_view prefix);

struct GRAPHICS_PARAMS {
	uint8_t device_id;
	int8_t api; // Negative = "use default API"
	BITDEPTH bitdepth;

	std::strong_ordering operator<=>(const GRAPHICS_PARAMS&) const = default;
};

// Validates and clamps [params] to the supported ranges before passing them on
// to GrpBackend_Init().
std::optional<GRAPHICS_PARAMS> Grp_Init(
	std::optional<const GRAPHICS_PARAMS> maybe_prev, GRAPHICS_PARAMS params
);

// Wraps screenshot handling around GrpBackend_Flip().
void Grp_Flip(void);
