/*
 *   Coordinate systems
 *
 */

#pragma once

#include <compare>

// Pixel-space coordinates
// -----------------------
// The unscaled output space in the game's original 640×480 resolution.

// X or Y value in unscaled pixel space. Relative to any origin.
using PIXEL_COORD = int;

// X/Y coordinate in unscaled pixel space. Relative to any origin.
struct PIXEL_POINT {
	PIXEL_COORD x;
	PIXEL_COORD y;

	PIXEL_POINT operator +(const PIXEL_POINT& other) const {
		return { (x + other.x), (y + other.y) };
	}
};

// Area size in unscaled pixel space.
// Using signed integers to avoid complicating the conversion into rectangle
// types, where signed coordinates often represent meaningful points outside
// the screen.
struct PIXEL_SIZE {
	PIXEL_COORD w;
	PIXEL_COORD h;

	explicit operator bool() const {
		return ((w > 0) && (h > 0));
	}

	PIXEL_SIZE operator -(const PIXEL_POINT& other) const {
		return { (w - other.x), (h - other.y) };
	}

	PIXEL_SIZE operator /(int divisor) const {
		return { (w / divisor), (h / divisor) };
	}

	std::strong_ordering operator <=>(const PIXEL_SIZE& other) const = default;
};

PIXEL_POINT& operator -=(PIXEL_POINT& self, const PIXEL_SIZE& other);

// Left-top-width-height rectangle in unscaled pixel space. Relative to any
// origin.
struct PIXEL_LTWH {
	PIXEL_COORD left;
	PIXEL_COORD top;
	PIXEL_COORD w;
	PIXEL_COORD h;
};

// Left-top-right-bottom rectangle in unscaled pixel space. Relative to any
// origin.
struct PIXEL_LTRB {
	PIXEL_COORD left;
	PIXEL_COORD top;
	PIXEL_COORD right;
	PIXEL_COORD bottom;

	PIXEL_LTRB() = default;
	PIXEL_LTRB(const PIXEL_LTRB&) = default;
	PIXEL_LTRB(PIXEL_LTRB&&) = default;
	PIXEL_LTRB& operator=(const PIXEL_LTRB&) = default;
	PIXEL_LTRB& operator=(PIXEL_LTRB&&) = default;
	PIXEL_LTRB(
		decltype(left) left,
		decltype(top) top,
		decltype(right) right,
		decltype(bottom) bottom
	) :
		left(left), top(top), right(right), bottom(bottom) {
	}

	PIXEL_LTRB(const PIXEL_LTWH& o) :
		left(o.left), top(o.top), right(o.left + o.w), bottom(o.top + o.h) {
	}

	PIXEL_SIZE Size() const {
		return { (right - left), (bottom - top) };
	}
};

using WINDOW_COORD = PIXEL_COORD;

// X/Y coordinate in unscaled game window space. The visible area ranges from
// (0, 0) to (639, 479) inclusive.
struct WINDOW_POINT : public PIXEL_POINT {
	WINDOW_POINT operator +(const PIXEL_POINT& other) const {
		return { (x + other.x), (y + other.y) };
	}
};

// Left-top-right-bottom rectangle in unscaled game window space. The visible
// area ranges from (0, 0) to (639, 479) inclusive.
struct WINDOW_LTRB : public PIXEL_LTRB {
	using PIXEL_LTRB::PIXEL_LTRB;
};
// -----------------------

// World-space coordinates
// -----------------------
// Not exclusively used for the playfield.

constexpr auto WORLD_COORD_BITS = 6;

using WORLD_COORD = int;

inline constexpr WORLD_COORD PixelToWorld(PIXEL_COORD v) {
	return (v << WORLD_COORD_BITS);
}

struct WORLD_POINT {
	WORLD_COORD x;
	WORLD_COORD y;

	[[gsl::suppress(type.6)]]
	WORLD_POINT() noexcept {
	}

	// World-space points should never be constructed from integer literals.
	// These literals may or may not be pixels, and usage code should not
	// assume or bother with [WORLD_COORD_BITS]. To construct a WORLD_POINT
	// from a pixel literal, explicitly construct a PIXEL_POINT first.
	WORLD_POINT(int x, int y) = delete;

	// TODO: Keeping this one around so that we can at least pass structure
	// fields while we gradually migrate the game to this structure, but it
	// should be `delete`d once we're done.
	WORLD_POINT(const WORLD_COORD* x, const WORLD_COORD* y) :
		x(*x), y(*y) {
	}

	WORLD_POINT(const PIXEL_POINT& pixel) :
		x(pixel.x << WORLD_COORD_BITS),
		y(pixel.y << WORLD_COORD_BITS) {
	}

	WORLD_POINT& operator -=(const WORLD_POINT& other) {
		x -= other.x;
		y -= other.y;
		return *this;
	}

	// Assumes this to be a centered coordinate, and calculates the
	// corresponding top-left coordinate based on the given size.
	PIXEL_POINT ToPixel(PIXEL_SIZE size_if_centered = { 0, 0 }) const {
		return {
			.x = ((x >> WORLD_COORD_BITS) - (size_if_centered.w >> 1)),
			.y = ((y >> WORLD_COORD_BITS) - (size_if_centered.h >> 1)),
		};
	}
};
// ---------------------------
