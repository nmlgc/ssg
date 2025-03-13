/*
 *   Common graphics interface, independent of a specific rendering API
 *
 */

#pragma once

import std.compat;
#include "game/cast.h"
#include "game/coords.h"
#include "game/enum_flags.h"
#include "game/pixelformat.h"
#include <assert.h>

// The backend will target a frame rate of
//
// 	[FRAME_TIME_TARGET] / [Grp_FPSDivisor]
//
// Setting this to 0 disables any frame rate limitation.
extern uint8_t Grp_FPSDivisor;

// Paletted graphics //
// ----------------- //

struct RGB {
	uint8_t r;
	uint8_t g;
	uint8_t b;

	constexpr RGBA WithAlpha(uint8_t a) const {
		return RGBA{ .r = r, .g = g, .b = b, .a = a };
	}

	constexpr bool operator==(const RGB& other) const = default;
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
		return RGB{
			.r = Cast::down<uint8_t>(r * 50u),
			.g = Cast::down<uint8_t>(g * 50u),
			.b = Cast::down<uint8_t>(b * 50u),
		};
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

enum class GRAPHICS_FULLSCREEN_FIT : uint8_t {
	// Scale to largest integer resolution
	INTEGER,

	// Scale to the largest resolution that fits the game's aspect ratio
	ASPECT,

	// Stretch to entire screen, disregarding the aspect ratio
	STRETCH,

	COUNT,
};

enum class GRAPHICS_PARAM_FLAGS : uint8_t {
	_HAS_BITFLAG_OPERATORS,
	FULLSCREEN = 0x01,
	FULLSCREEN_EXCLUSIVE = 0x02,

	// A GRAPHICS_FULLSCREEN_FIT value
	FULLSCREEN_FIT = (std::to_underlying(GRAPHICS_FULLSCREEN_FIT::COUNT) << 2),

	// Render at the window's resolution instead of at [GRP_RES]
	SCALE_GEOMETRY = 0x10,

	MASK = (
		FULLSCREEN | FULLSCREEN_EXCLUSIVE | FULLSCREEN_FIT | SCALE_GEOMETRY
	),
};

struct GRAPHICS_FULLSCREEN_FLAGS {
	bool fullscreen;
	bool exclusive;
	GRAPHICS_FULLSCREEN_FIT fit;

	std::strong_ordering operator<=>(
		const GRAPHICS_FULLSCREEN_FLAGS&
	) const = default;
};

constexpr auto GRAPHICS_TOPLEFT_UNDEFINED = (
	std::numeric_limits<int16_t>::min
)();

struct GRAPHICS_PARAMS {
	GRAPHICS_PARAM_FLAGS flags;
	uint8_t device_id;
	int8_t api; // Negative = "use default API"
	uint8_t window_scale_4x; // Scale factor in window mode ×4. 0 = fit display.

	// Across all displays. Can be [GRAPHICS_TOPLEFT_UNDEFINED], in which case
	// the window backend should pick a reasonable default position.
	int16_t left;
	int16_t top;

	BITDEPTH bitdepth;

	std::strong_ordering operator<=>(const GRAPHICS_PARAMS&) const = default;

	GRAPHICS_FULLSCREEN_FLAGS FullscreenFlags(void) const;
	bool ScaleGeometry(void) const;
	uint8_t Scale4x(void) const;
	WINDOW_SIZE ScaledRes(void) const;

	void SetFlag(
		GRAPHICS_PARAM_FLAGS flag,
		std::underlying_type_t<GRAPHICS_PARAM_FLAGS> value
	);
};

// Returns the maximum 4× scaling factor for the game window on the current
// display.
uint8_t Grp_WindowScale4xMax(void);

struct GRAPHICS_INIT_RESULT {
	GRAPHICS_PARAMS live;
	bool reload_surfaces;

	static std::optional<GRAPHICS_INIT_RESULT> From(
		std::optional<GRAPHICS_PARAMS>&& o
	) {
		return o.transform([](auto&& o) {
			return GRAPHICS_INIT_RESULT{ .live = o, .reload_surfaces = false };
		});
	}
};

// Validates and clamps [params] to the supported ranges before passing them on
// to GrpBackend_Init().
std::optional<GRAPHICS_INIT_RESULT> Grp_Init(
	std::optional<const GRAPHICS_PARAMS> maybe_prev, GRAPHICS_PARAMS params
);

// Calls Grp_Init() with the given parameters and tries the remaining devices
// and APIs failure. Returns the actual configuration the backend was
// initialized with, or `std::nullopt` on failure.
std::optional<GRAPHICS_INIT_RESULT> Grp_InitOrFallback(GRAPHICS_PARAMS params);

// Wraps screenshot handling around GrpBackend_Flip().
void Grp_Flip(void);
