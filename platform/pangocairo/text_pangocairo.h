/*
 *   Text rendering via Pango/Cairo
 *
 */

#pragma once

#include "game/text_packed.h"
#include "platform/graphics_backend.h"

class TEXTRENDER_PANGOCAIRO_SESSION {
protected:
	PIXEL_POINT tex_origin;
	PIXEL_SIZE size;
	FONT_ID font_cur = FONT_ID::COUNT;
	RGB color_cur = { 0, 0, 0 };

	class PIXELACCESS {
		uint8_t *buf;
		int stride;

		uint32_t& PixelAt(const PIXEL_POINT& xy_rel);

	public:
		uint32_t GetRaw(const PIXEL_POINT& xy_rel);
		void SetRaw(const PIXEL_POINT& xy_rel, uint32_t col);

		RGB Get(const PIXEL_POINT& xy_rel);
		void Set(const PIXEL_POINT& xy_rel, const RGB col);

		PIXELACCESS(void);
		~PIXELACCESS(void);
	};
	static_assert(TEXTRENDER_SESSION_PIXELACCESS<PIXELACCESS>);

public:
	PIXEL_SIZE RectSize(void) const;
	void SetFont(FONT_ID font);
	void SetColor(const RGB& color);
	PIXEL_SIZE Extent(Narrow::string_view str);
	void Put(
		const PIXEL_POINT& topleft_rel,
		Narrow::string_view str,
		std::optional<RGB> color = std::nullopt
	);
	auto PixelAccess(std::invocable<PIXELACCESS&> auto f) {
		PIXELACCESS p;
		return f(p);
	}

	TEXTRENDER_PANGOCAIRO_SESSION(const PIXEL_LTWH rect);
	~TEXTRENDER_PANGOCAIRO_SESSION();
};

static_assert(TEXTRENDER_SESSION<TEXTRENDER_PANGOCAIRO_SESSION>);

class TEXTRENDER_PANGOCAIRO : public TEXTRENDER_PACKED<
	TEXTRENDER_PANGOCAIRO_SESSION
> {
	friend class TEXTRENDER_PACKED;

	std::optional<TEXTRENDER_PANGOCAIRO_SESSION> Session(
		TEXTRENDER_RECT_ID rect_id
	);

public:
	void WipeBeforeNextRender();
	PIXEL_SIZE TextExtent(FONT_ID font, Narrow::string_view str);
};

static_assert(TEXTRENDER<TEXTRENDER_PANGOCAIRO>);

extern TEXTRENDER_PANGOCAIRO TextObj;
