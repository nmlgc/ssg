/*
 *   Platform-specific text rendering backend
 *
 */

#pragma once

#include "game/graphics.h"

// Concept for pixel access within a text rendering session. Offers access
// using both RGB colors and the raw underlying format.
template <class T> concept TEXTRENDER_SESSION_PIXELACCESS = requires(
	T t, PIXEL_POINT xy_rel, decltype(t.GetRaw(xy_rel)) color_raw, RGB color
) {
	{ t.GetRaw(xy_rel) } -> std::same_as<decltype(color_raw)>;
	t.SetRaw(xy_rel, color_raw);

	{ t.Get(xy_rel) } -> std::same_as<RGB>;
	t.Set(xy_rel, color);
};

#ifdef WIN32
	#include "platform/windows/text_gdi.h"

	extern TEXTRENDER_GDI TextObj;
#endif

// Shuts down the backend, deleting all fonts.
void TextBackend_Cleanup(void);
