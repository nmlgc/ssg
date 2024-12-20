/*
 *   Text rendering via GDI
 *
 */

#pragma once

#include "game/text_packed.h"
#include "platform/graphics_backend.h"
#include <assert.h>
#include <windows.h>

class TEXTRENDER_GDI_SESSION {
protected:
	const ENUMARRAY<HFONT, FONT_ID>& fonts;

	// HFONT would require a cast of the value returned from SelectObject().
	std::optional<HGDIOBJ> font_initial = std::nullopt;

	// A COLORREF created with the RGB macro always has 0x00 in the topmost 8
	// bits.
	uint32_t color_cur = -1;

	FONT_ID font_cur = FONT_ID::COUNT;

	class PIXELACCESS {
		const PIXEL_LTWH rect;

	public:
		uint32_t GetRaw(const PIXEL_POINT& xy_rel);
		void SetRaw(const PIXEL_POINT& xy_rel, uint32_t col);

		RGBA Get(const PIXEL_POINT& xy_rel);
		void Set(const PIXEL_POINT& xy_rel, const RGBA col);

		PIXELACCESS(const PIXEL_LTWH rect) : rect(rect) {
		}
	};
	static_assert(TEXTRENDER_SESSION_PIXELACCESS<PIXELACCESS>);

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
	auto PixelAccess(std::invocable<PIXELACCESS&> auto f) {
		PIXELACCESS p = { rect };
		return f(p);
	}

	TEXTRENDER_GDI_SESSION(
		const PIXEL_LTWH& rect, HDC hdc, const ENUMARRAY<HFONT, FONT_ID>& fonts
	);
	~TEXTRENDER_GDI_SESSION();
};

static_assert(TEXTRENDER_SESSION<TEXTRENDER_GDI_SESSION>);

class TEXTRENDER_GDI : public TEXTRENDER_PACKED {
	ENUMARRAY<HFONT, FONT_ID>& fonts;

	bool Wipe();

	// Common rendering preparation code.
	std::optional<TEXTRENDER_GDI_SESSION> PreparePrerender(
		TEXTRENDER_RECT_ID rect_id
	);

public:
	// Can share the font array with other GDI renderers.
	TEXTRENDER_GDI(decltype(fonts)& fonts) : fonts(fonts) {
	}

	void WipeBeforeNextRender();

	PIXEL_SIZE TextExtent(FONT_ID font, Narrow::string_view str);

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
	);

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

static_assert(TEXTRENDER<TEXTRENDER_GDI>);
