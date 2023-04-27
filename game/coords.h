/*
 *   Coordinate systems
 *
 */

#pragma once

// Pixel-space coordinates
// -----------------------
// The unscaled output space in the game's original 640×480 resolution.

// X or Y value in unscaled pixel space.
using PIXEL_COORD = int;

// Area size in unscaled pixel space.
// Using signed integers to avoid complicating the conversion into rectangle
// types, where signed coordinates often represent meaningful points outside
// the screen.
struct PIXEL_SIZE {
	PIXEL_COORD w;
	PIXEL_COORD h;
};
// -----------------------
