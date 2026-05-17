/*
 *   Coordinate systems
 *
 */

#pragma once

import std;

// Pixel-space coordinates
// -----------------------
// The unscaled output space in the game's native resolution.

// X or Y value in unscaled pixel space. Relative to any origin.
using PIXEL_COORD = int;

// X/Y coordinate in unscaled pixel space. Relative to any origin.
template <class Coord> struct PIXEL_POINT_BASE {
	Coord x;
	Coord y;

	// Explicit integer division.
	PIXEL_POINT_BASE DivInt(int scalar) const {
		return {
			.x = static_cast<Coord>(static_cast<int>(x) / scalar),
			.y = static_cast<Coord>(static_cast<int>(y) / scalar),
		};
	}

	constexpr PIXEL_POINT_BASE operator+(const PIXEL_POINT_BASE& other) const {
		return { (x + other.x), (y + other.y) };
	}

	PIXEL_POINT_BASE& operator-=(const PIXEL_POINT_BASE& other) {
		this->x -= other.x;
		this->y -= other.y;
		return *this;
	}

	std::partial_ordering operator<=>(const PIXEL_POINT_BASE&) const = default;
};

// Area size in unscaled pixel space.
// Using signed integers to avoid complicating the conversion into rectangle
// types, where signed coordinates often represent meaningful points outside
// the screen.
template <class Coord> struct PIXEL_SIZE_BASE {
	Coord w;
	Coord h;

	explicit operator bool() const {
		return ((w > 0) && (h > 0));
	}

	constexpr PIXEL_SIZE_BASE operator-(
		const PIXEL_POINT_BASE<Coord>& other
	) const {
		return { (w - other.x), (h - other.y) };
	}

	constexpr PIXEL_SIZE_BASE operator*(int factor) const {
		return { (w * factor), (h * factor) };
	}

	constexpr PIXEL_SIZE_BASE operator/(int divisor) const {
		return { (w / divisor), (h / divisor) };
	}

	constexpr PIXEL_SIZE_BASE operator/(const PIXEL_SIZE_BASE& other) const {
		return { (w / other.w), (h / other.h) };
	}

	PIXEL_SIZE_BASE& operator+=(const PIXEL_SIZE_BASE& other) {
		w += other.w;
		h += other.h;
		return *this;
	}

	std::partial_ordering operator<=>(const PIXEL_SIZE_BASE&) const = default;
};

template <class Coord> PIXEL_POINT_BASE<Coord> operator+(
	const PIXEL_POINT_BASE<Coord>& self, const PIXEL_SIZE_BASE<Coord>& other
) {
	return { .x = (self.x + other.w), .y = (self.y + other.h) };
}

template <class Coord> PIXEL_POINT_BASE<Coord> operator-(
	const PIXEL_POINT_BASE<Coord>& self, const PIXEL_SIZE_BASE<Coord>& other
) {
	return { .x = (self.x - other.w), .y = (self.y - other.h) };
}

template <class Coord> PIXEL_POINT_BASE<Coord>& operator+=(
	PIXEL_POINT_BASE<Coord>& self, const PIXEL_SIZE_BASE<Coord>& other
) {
	self.x += other.w;
	self.y += other.h;
	return self;
}

template <class Coord> PIXEL_POINT_BASE<Coord>& operator-=(
	PIXEL_POINT_BASE<Coord>& self, const PIXEL_SIZE_BASE<Coord>& other
) {
	self.x -= other.w;
	self.y -= other.h;
	return self;
}

// Left-top-width-height rectangle in unscaled pixel space. Relative to any
// origin.
template <class Coord> struct PIXEL_LTWH_BASE {
	Coord left = 0;
	Coord top = 0;
	Coord w = 0;
	Coord h = 0;

	constexpr PIXEL_LTWH_BASE() = default;
	constexpr PIXEL_LTWH_BASE(const PIXEL_LTWH_BASE&) = default;
	constexpr PIXEL_LTWH_BASE(PIXEL_LTWH_BASE&&) = default;
	constexpr PIXEL_LTWH_BASE& operator=(const PIXEL_LTWH_BASE&) = default;
	constexpr PIXEL_LTWH_BASE& operator=(PIXEL_LTWH_BASE&&) = default;
	constexpr PIXEL_LTWH_BASE(Coord left, Coord top, Coord w, Coord h)
		: left(left), top(top), w(w), h(h)
	{
	}

	constexpr PIXEL_LTWH_BASE operator+(
		const PIXEL_POINT_BASE<Coord>& other
	) const {
		return { (left + other.x), (top + other.y), w, h };
	}
};

// Left-top-right-bottom rectangle in unscaled pixel space. Relative to any
// origin.
template <class Coord> struct PIXEL_LTRB_BASE {
	Coord left;
	Coord top;
	Coord right;
	Coord bottom;

	PIXEL_LTRB_BASE() = default;
	PIXEL_LTRB_BASE(const PIXEL_LTRB_BASE&) = default;
	PIXEL_LTRB_BASE(PIXEL_LTRB_BASE&&) = default;
	PIXEL_LTRB_BASE& operator=(const PIXEL_LTRB_BASE&) = default;
	PIXEL_LTRB_BASE& operator=(PIXEL_LTRB_BASE&&) = default;
	constexpr PIXEL_LTRB_BASE(
		decltype(left) left,
		decltype(top) top,
		decltype(right) right,
		decltype(bottom) bottom
	) :
		left(left), top(top), right(right), bottom(bottom) {
	}
	constexpr PIXEL_LTRB_BASE(
		const PIXEL_POINT_BASE<Coord>& topleft,
		const PIXEL_SIZE_BASE<Coord>& size
	) :
		left(topleft.x),
		top(topleft.y),
		right(topleft.x + size.w),
		bottom(topleft.y + size.h) {
	}

	constexpr PIXEL_LTRB_BASE(const PIXEL_LTWH_BASE<Coord>& o) :
		left(o.left), top(o.top), right(o.left + o.w), bottom(o.top + o.h) {
	}

	PIXEL_SIZE_BASE<Coord> Size() const {
		return { (right - left), (bottom - top) };
	}
};

using WINDOW_COORD = PIXEL_COORD;

// X/Y coordinate in unscaled game window space. The visible area ranges from
// (0, 0) inclusive to [GRP_RES] exclusive.
template <
	class Coord
> struct WINDOW_POINT_BASE : public PIXEL_POINT_BASE<Coord> {
	constexpr WINDOW_POINT_BASE operator+(
		const PIXEL_POINT_BASE<Coord>& other
	) const {
		return { (this->x + other.x), (this->y + other.y) };
	}

	WINDOW_POINT_BASE operator/(Coord scalar) const {
		return { (this->x / scalar), (this->y / scalar) };
	}
};

// Area size in unscaled game window space.
template <class Coord> using WINDOW_SIZE_BASE = PIXEL_SIZE_BASE<Coord>;

// Left-top-width-height rectangle in unscaled game window space. The visible
// area ranges from (0, 0) inclusive to [GRP_RES] exclusive.
template <class Coord> struct WINDOW_LTWH_BASE : public PIXEL_LTWH_BASE<Coord> {
	using PIXEL_LTWH_BASE<Coord>::PIXEL_LTWH_BASE;
};

// Left-top-right-bottom rectangle in unscaled game window space. The visible
// area ranges from (0, 0) inclusive to [GRP_RES] exclusive.
template <class Coord> struct WINDOW_LTRB_BASE : public PIXEL_LTRB_BASE<Coord> {
	using PIXEL_LTRB_BASE<Coord>::PIXEL_LTRB_BASE;

	constexpr WINDOW_LTRB_BASE(const WINDOW_LTWH_BASE<Coord>& o) :
		PIXEL_LTRB_BASE<Coord>(o) {
	}
};

using PIXEL_POINT = PIXEL_POINT_BASE<PIXEL_COORD>;
using PIXEL_SIZE = PIXEL_SIZE_BASE<PIXEL_COORD>;
using PIXEL_LTWH = PIXEL_LTWH_BASE<PIXEL_COORD>;
using PIXEL_LTRB = PIXEL_LTRB_BASE<PIXEL_COORD>;
using WINDOW_POINT = WINDOW_POINT_BASE<WINDOW_COORD>;
using WINDOW_SIZE = WINDOW_SIZE_BASE<WINDOW_COORD>;
using WINDOW_LTWH = WINDOW_LTWH_BASE<WINDOW_COORD>;
using WINDOW_LTRB = WINDOW_LTRB_BASE<WINDOW_COORD>;
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

	#pragma warning(suppress : 26495) // type.6
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
