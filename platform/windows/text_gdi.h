/*
 *   Text rendering via GDI
 *
 */

#pragma once

#include "game/enum_array.h"
#include "game/text_packed.h"
#include <assert.h>
#include <span>

// Only required for the HDC, HFONT, and HGDIOBJ types, which are basically
// void*.
#include <windows.h>

// Un-templated session code
class TEXTRENDER_GDI_SESSION_BASE {
protected:
	const std::span<HFONT> fonts;

	// HFONT would require a cast of the value returned from SelectObject().
	std::optional<HGDIOBJ> font_initial = std::nullopt;

	// A COLORREF created with the RGB macro always has 0x00 in the topmost 8
	// bits.
	uint32_t color_cur = -1;

	void SetFont(size_t id);

public:
	const PIXEL_LTWH rect;
	HDC hdc;

	void SetColor(const RGBA& color);
	void Put(
		const PIXEL_POINT& topleft_rel,
		std::string_view str,
		std::optional<RGBA> color = std::nullopt
	);

	TEXTRENDER_GDI_SESSION_BASE(
		const PIXEL_LTWH& rect, HDC hdc, const std::span<HFONT> fonts
	);
	~TEXTRENDER_GDI_SESSION_BASE();
};

template <
	class Graphics, class Surface, ENUMARRAY_ID FontID
> class TEXTRENDER_GDI : public TEXTRENDER_PACKED {
	Graphics& graphics;
	Surface& surf;

	class SESSION : public TEXTRENDER_GDI_SESSION_BASE {
		FontID font_cur = FontID::COUNT;

	public:
		using TEXTRENDER_GDI_SESSION_BASE::TEXTRENDER_GDI_SESSION_BASE;

		void SetFont(FontID font) {
			if(font_cur != font) {
				TEXTRENDER_GDI_SESSION_BASE::SetFont(size_t(font));
				font_cur = font;
			}
		}
	};

	bool Wipe() {
		return (
			graphics.SurfaceCreateBlank(surf, bounds) &&
			graphics.SurfaceSetColorKey(surf, { 0x00, 0x00, 0x00 }) &&
			TEXTRENDER_PACKED::Wipe()
		);
	}

	// Common rendering preparation code.
	std::optional<SESSION> PreparePrerender(TEXTRENDER_RECT_ID rect_id) {
		if((surf.size != bounds) && !Wipe()) {
			return std::nullopt;
		}
		assert(rect_id < rects.size());
		return SESSION{
			rects[rect_id].rect,
			surf.dc,
			std::span<HFONT>{ fonts.data(), fonts.size() }
		};
	}

public:
	ENUMARRAY<HFONT, FontID>& fonts;

	// Can share the font array with other GDI renderers.
	TEXTRENDER_GDI(Graphics& graphics, Surface& surf, decltype(fonts)& fonts) :
		graphics(graphics), surf(surf), fonts(fonts) {
	}

	bool Prerender(
		TEXTRENDER_RECT_ID rect_id,
		TEXTRENDER_SESSION_FUNC<SESSION, FontID> auto func
	) {
		auto maybe_session = PreparePrerender(rect_id);
		if(!maybe_session) {
			return false;
		}
		auto& session = maybe_session.value();
		return graphics.SurfaceEdit(surf, [&session, func](auto, auto) {
			func(std::ref(session).get());
			return true;
		}, session.rect);
	}

	bool Blit(
		WINDOW_POINT dst,
		TEXTRENDER_RECT_ID rect_id,
		std::optional<PIXEL_LTWH> subrect = std::nullopt
	) {
		PIXEL_LTRB rect = Subrect(rect_id, subrect);
		return graphics.SurfaceBlit(dst, surf, rect);
	}

	bool Render(
		WINDOW_POINT dst,
		TEXTRENDER_RECT_ID rect_id,
		std::string_view contents,
		TEXTRENDER_SESSION_FUNC<SESSION, FontID> auto func,
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
