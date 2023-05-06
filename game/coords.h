/*
 *   Coordinate systems
 *
 */

#pragma once

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
};

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
};

// X/Y coordinate in unscaled game window space. The visible area ranges from
// (0, 0) to (639, 479) inclusive.
struct WINDOW_POINT : public PIXEL_POINT {
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

struct WORLD_POINT {
	WORLD_COORD x;
	WORLD_COORD y;

	WORLD_POINT() {
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
