/*
 *   Common font rendering interface, independent of a specific rasterizer
 *
 */

#pragma once

#include "game/coords.h"
#include "game/enum_array.h"
#include "game/graphics.h"
#include "game/narrow.h"
#include <optional>
#include <string_view>

using TEXTRENDER_RECT_ID = unsigned int;

// Concept for a text render session on a single rectangle, abstracting away
// the rasterizer.
template <class T, class FontID> concept TEXTRENDER_SESSION = (
	ENUMARRAY_ID<FontID> && requires (
		T t,
		PIXEL_POINT topleft_rel,
		Narrow::string_view str,
		RGBA color,
		FontID font
	) {
		{ t.rect } -> std::same_as<const PIXEL_LTWH&>;

		t.SetFont(font);
		t.SetColor(color);

		// Calculates the width and height of [str] as rendered with the
		// current font.
		{ t.Extent(str) } -> std::same_as<PIXEL_SIZE>;

		// Text display with the current color and font. [str] can be either
		// UTF-8 or Shift-JIS.
		t.Put(topleft_rel, str);

		// Convenience overload to change the color before rendering the text.
		// (Not adding one for the font, since the ID is templated. This
		// allows Put() to be fully implemented within a type-erased base
		// class.)
		t.Put(topleft_rel, str, color);
	}
);

// Horizontally [str] on [s]'s rectangle.
PIXEL_COORD TextLayoutXCenter(auto& s, Narrow::string_view str) {
	return ((s.rect.w - s.Extent(str).w) / 2);
}

// Concept that describes valid text rendering session functors in game code.
template <typename F, class Session, class FontID>
concept TEXTRENDER_SESSION_FUNC = (
	TEXTRENDER_SESSION<Session, FontID> &&
	requires(F f, Session& s) {
		{ f(s) };
	}
);

// Just here to keep the TEXTRENDER concept from requiring an impossible
// template parameter for the session functor.
template <class FontID> struct TEXTRENDER_SESSION_FUNC_ARCHETYPE {
	TEXTRENDER_SESSION_FUNC_ARCHETYPE() = delete;
	void operator()(TEXTRENDER_SESSION<FontID> auto& s) {
	}
};

// Concept for a text rendering backend.
template <class T, class FontID> concept TEXTRENDER = requires(
	T t,
	FontID font,
	PIXEL_SIZE size,
	WINDOW_POINT dst,
	Narrow::string_view contents,
	TEXTRENDER_RECT_ID rect_id,
	TEXTRENDER_SESSION_FUNC_ARCHETYPE<FontID>& func,
	std::optional<PIXEL_LTWH> subrect
) {
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

	// Forces the next (pre-)render call to clear all image data and recreate
	// the surface with the necessary size, but retaining all registered
	// rectangles. Necessary for mode switches.
	t.WipeBeforeNextRender();

	// Calculates the width and height of [contents] as rendered with the given
	// font.
	{ t.TextExtent(font, contents) } -> std::same_as<PIXEL_SIZE>;

	// Retained interface
	// ------------------
	// Explicit prerendering with later blitting.

	{ t.Prerender(rect_id, func) } -> std::same_as<bool>;
	t.Blit(dst, rect_id, subrect);
	// ------------------

	// Immediate interface
	// -------------------
	// Associates the results of [func] with the given [contents], thus caching
	// the result rendered to [rect_id]. [func] will only be called again if
	// the [contents] changed.

	t.Render(dst, rect_id, contents, func, subrect);
	// -------------------
};
