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
};

// Area size in unscaled pixel space.
// Using signed integers to avoid complicating the conversion into rectangle
// types, where signed coordinates often represent meaningful points outside
// the screen.
struct PIXEL_SIZE {
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
};

// X/Y coordinate in unscaled game window space. The visible area ranges from
// (0, 0) to (639, 479) inclusive.
struct WINDOW_POINT : public PIXEL_POINT {
};

// Left-top-right-bottom rectangle in unscaled game window space. The visible
// area ranges from (0, 0) to (639, 479) inclusive.
struct WINDOW_LTRB : public PIXEL_LTRB {
};
// -----------------------
