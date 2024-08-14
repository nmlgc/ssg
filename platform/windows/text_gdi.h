/*
 *   Text rendering via GDI
 *
 */

#pragma once

#include "game/text_packed.h"
#include "platform/windows/surface_gdi.h"
#include "platform/graphics_backend.h"
#include <assert.h>

PIXEL_SIZE TextGDIExtent(
	HDC hdc, std::optional<HFONT> font, Narrow::string_view str
);

class TEXTRENDER_GDI_SESSION {
protected:
	const ENUMARRAY<HFONT, FONT_ID>& fonts;

	// HFONT would require a cast of the value returned from SelectObject().
	std::optional<HGDIOBJ> font_initial = std::nullopt;

	// A COLORREF created with the RGB macro always has 0x00 in the topmost 8
	// bits.
	uint32_t color_cur = -1;

	FONT_ID font_cur = FONT_ID::COUNT;

public:
	const PIXEL_LTWH rect;
	HDC hdc;

	void SetFont(FONT_ID font);
	void SetColor(const RGBA& color);
	PIXEL_SIZE Extent(Narrow::string_view str);
	void Put(
		const PIXEL_POINT& topleft_rel,
		Narrow::string_view str,
		std::optional<RGBA> color = std::nullopt
	);

	TEXTRENDER_GDI_SESSION(
		const PIXEL_LTWH& rect, HDC hdc, const ENUMARRAY<HFONT, FONT_ID>& fonts
	);
	~TEXTRENDER_GDI_SESSION();
};

static_assert(TEXTRENDER_SESSION<TEXTRENDER_GDI_SESSION>);

template <
	class Graphics, class Surface
> class TEXTRENDER_GDI : public TEXTRENDER_PACKED {
	Graphics& graphics;
	Surface& surf;
	ENUMARRAY<HFONT, FONT_ID>& fonts;

	bool Wipe() {
		return (
			GrpSurface_GDIText_Create(bounds, { 0x00, 0x00, 0x00 }) &&
			TEXTRENDER_PACKED::Wipe()
		);
	}

	// Common rendering preparation code.
	std::optional<TEXTRENDER_GDI_SESSION> PreparePrerender(
		TEXTRENDER_RECT_ID rect_id
	) {
		auto& surf = GrpSurface_GDIText_Surface();
		if((surf.size != bounds) && !Wipe()) {
			return std::nullopt;
		}
		assert(rect_id < rects.size());
		return TEXTRENDER_GDI_SESSION{ rects[rect_id].rect, surf.dc, fonts };
	}

public:
	// Can share the font array with other GDI renderers.
	TEXTRENDER_GDI(Graphics& graphics, decltype(fonts)& fonts) :
		graphics(graphics), fonts(fonts), surf(DxSurf[SURFACE_ID::TEXT]) {
	}

	void WipeBeforeNextRender() {
		TEXTRENDER_PACKED::Wipe();

		// This also skips the needless creation of an uninitialized surface
		// during DirectDraw's init function.
		GrpSurface_GDIText_Surface().size = { 0, 0 };
	}

	PIXEL_SIZE TextExtent(FONT_ID font, Narrow::string_view str) {
		return TextGDIExtent(GrpSurface_GDIText_Surface().dc, fonts[font], str);
	}

	bool Prerender(
		TEXTRENDER_RECT_ID rect_id,
		TEXTRENDER_SESSION_FUNC<TEXTRENDER_GDI_SESSION> auto func
	) {
		auto maybe_session = PreparePrerender(rect_id);
		if(!maybe_session) {
			return false;
		}
		auto& session = maybe_session.value();
		func(session);
		return GrpSurface_GDIText_Update(session.rect);
	}

	bool Blit(
		WINDOW_POINT dst,
		TEXTRENDER_RECT_ID rect_id,
		std::optional<PIXEL_LTWH> subrect = std::nullopt
	) {
		const PIXEL_LTRB rect = Subrect(rect_id, subrect);
		return graphics.SurfaceBlit(dst, surf, rect);
	}

	bool Render(
		WINDOW_POINT dst,
		TEXTRENDER_RECT_ID rect_id,
		Narrow::string_view contents,
		TEXTRENDER_SESSION_FUNC<TEXTRENDER_GDI_SESSION> auto func,
		std::optional<PIXEL_LTWH> subrect = std::nullopt
	) {
		assert(rect_id < rects.size());
		auto& rect = rects[rect_id];
		if(rect.contents != contents) {
			if(!Prerender(rect_id, func)) {
				return false;
			}
			rect.contents = contents;
		}
		return Blit(dst, rect_id, subrect);
	}
};
