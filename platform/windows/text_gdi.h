/*
 *   Text rendering via GDI
 *
 */

#pragma once

#define WIN32_LEAN_AND_MEAN

#include "game/enum_array.h"
#include "game/text_packed.h"
#include <assert.h>
#include <windows.h>

template <
	class Graphics, class Surface, ENUMARRAY_ID FontID
> class TEXTRENDER_GDI : public TEXTRENDER_PACKED {
	Graphics& graphics;
	Surface& surf;

	// Common rendering preparation code.
	std::optional<PIXEL_LTWH> PreparePrerender(TEXTRENDER_RECT_ID rect_id) {
		if(!graphics.SurfaceSetColorKey(surf, { 0x00, 0x00, 0x00 })) {
			return std::nullopt;
		}
		assert(rect_id < rects.size());
		return rects[rect_id];
	}

public:
	ENUMARRAY<HFONT, FontID>& fonts;

	// Can share the font array with other GDI renderers.
	TEXTRENDER_GDI(Graphics& graphics, Surface& surf, decltype(fonts)& fonts) :
		graphics(graphics), surf(surf), fonts(fonts) {
	}

	bool Prerender(TEXTRENDER_RECT_ID rect_id, auto func) {
		auto maybe_rect = PreparePrerender(rect_id);
		if(!maybe_rect) {
			return false;
		}
		return graphics.SurfaceEdit(surf, func, maybe_rect.value());
	}

	bool Blit(
		WINDOW_POINT dst,
		TEXTRENDER_RECT_ID rect_id,
		std::optional<PIXEL_LTWH> subrect = std::nullopt
	) {
		PIXEL_LTRB rect = Subrect(rect_id, subrect);
		return graphics.SurfaceBlit(dst, surf, rect);
	}
};
