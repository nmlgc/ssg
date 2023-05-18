/*
 *   Common font rendering interface, independent of a specific rasterizer
 *
 */

#pragma once

#include "game/coords.h"
#include <optional>

using TEXTRENDER_RECT_ID = unsigned int;

// Concept for a text rendering backend.
template <class T> concept TEXTRENDER = requires(T t, PIXEL_SIZE size) {
	// Rectangle management
	// --------------------

	// Registers a new text rectangle of the given size, and returns a unique
	// ID to it on success. This ID is valid until the next Clear().
	// This can resize the text surface on the next (pre-)rendering call if the
	// current one can't fit the new rectangle, which will clear any previously
	// rendered text rectangles. Therefore, it should only be called during
	// game state initialization.
	{ t.Register(size) } -> std::same_as<TEXTRENDER_RECT_ID>;

	// Invalidates all registered text rectangles.
	t.Clear();
	// --------------------
};
