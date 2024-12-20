/*
 *   Text rendering via GDI
 *
 */

#include "platform/text_backend.h"
#include "platform/windows/surface_gdi.h"
#include "platform/windows/utf.h"

extern const ENUMARRAY<LOGFONTW, FONT_ID> FontSpecs;

static class {
	ENUMARRAY<HFONT, FONT_ID> arr;

public:
	HFONT ForID(FONT_ID font)
	{
		if(!arr[font]) {
			arr[font] = CreateFontIndirectW(&FontSpecs[font]);
		}
		return arr[font];
	}

	void Cleanup(void)
	{
		for(auto& font : arr) {
			if(font) {
				DeleteObject(font);
				font = nullptr;
			}
		}
	}
} Fonts;

PIXEL_SIZE TextGDIExtent(std::optional<HFONT> font, Narrow::string_view str)
{
	const auto hdc = GrpSurface_GDIText_Surface().dc;
	const auto font_prev = (font ? SelectObject(hdc, font.value()) : nullptr);
	const auto ret = UTF::WithUTF16<PIXEL_SIZE>(
		str, [&](const std::wstring_view str_w) {
			RECT r = { 0, 0, 0, 0 };
			constexpr UINT flags = (DT_CALCRECT | DT_SINGLELINE);
			if(!DrawTextW(hdc, str_w.data(), str_w.size(), &r, flags)) {
				return PIXEL_SIZE{ 0, 0 };
			}
			return PIXEL_SIZE{ r.right, r.bottom };
		}
	).value_or(PIXEL_SIZE{ 0, 0 });
	if(font && font_prev) {
		SelectObject(hdc, font_prev);
	}
	return ret;
}

uint32_t TEXTRENDER_GDI_SESSION::PIXELACCESS::GetRaw(const PIXEL_POINT& xy_rel)
{
	const auto hdc = GrpSurface_GDIText_Surface().dc;
	return GetPixel(hdc, (rect.left + xy_rel.x), (rect.top + xy_rel.y));
}

void TEXTRENDER_GDI_SESSION::PIXELACCESS::SetRaw(
	const PIXEL_POINT& xy_rel, uint32_t color
)
{
	const auto hdc = GrpSurface_GDIText_Surface().dc;
	SetPixelV(hdc, (rect.left + xy_rel.x), (rect.top + xy_rel.y), color);
}

RGBA TEXTRENDER_GDI_SESSION::PIXELACCESS::Get(const PIXEL_POINT& xy_rel)
{
	const auto ret = GetRaw(xy_rel);
	return RGBA{
		.r = GetRValue(ret), .g = GetGValue(ret), .b = GetBValue(ret)
	};
}

void TEXTRENDER_GDI_SESSION::PIXELACCESS::Set(
	const PIXEL_POINT& xy_rel, const RGBA color
)
{
	SetRaw(xy_rel, RGB(color.r, color.g, color.b));
}

TEXTRENDER_GDI_SESSION::TEXTRENDER_GDI_SESSION(const PIXEL_LTWH& rect) :
	rect(rect)
{
	const auto hdc = GrpSurface_GDIText_Surface().dc;

	// Clear the rectangle, for two reasons:
	// • The caller is free to render multiple transparent strings on top
	//   of each other, so we can't rely on SetBkMode(hdc, OPAQUE).
	// • The next text might be shorter than the previous one in this
	//   rectangle.
	const PIXEL_LTRB ltrb = rect;
	const RECT r = { ltrb.left, ltrb.top, ltrb.right, ltrb.bottom };
	auto black = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
	const auto rect_filled_with_black = FillRect(hdc, &r, black);
	const auto background_mode_set_to_transparent = SetBkMode(hdc, TRANSPARENT);
	assert(rect_filled_with_black);
	assert(background_mode_set_to_transparent);
}

TEXTRENDER_GDI_SESSION::~TEXTRENDER_GDI_SESSION()
{
	if(font_initial) {
		const auto hdc = GrpSurface_GDIText_Surface().dc;
		SelectObject(hdc, font_initial.value());
	}
}

void TEXTRENDER_GDI_SESSION::SetFont(FONT_ID font)
{
	if(font_cur != font) {
		const auto hdc = GrpSurface_GDIText_Surface().dc;
		auto font_prev = SelectObject(hdc, Fonts.ForID(font));
		if(!font_initial) {
			font_initial = font_prev;
		}
	}
}

void TEXTRENDER_GDI_SESSION::SetColor(const RGBA& color)
{
	const COLORREF color_gdi = RGB(color.r, color.g, color.b);
	if(color_cur != color_gdi) {
		const auto hdc = GrpSurface_GDIText_Surface().dc;
		SetTextColor(hdc, color_gdi);
		color_cur = color_gdi;
	}
}

PIXEL_SIZE TEXTRENDER_GDI_SESSION::Extent(Narrow::string_view str)
{
	return TextGDIExtent(std::nullopt, str);
}

void TEXTRENDER_GDI_SESSION::Put(
	const PIXEL_POINT& topleft_rel,
	Narrow::string_view str,
	std::optional<RGBA> color
)
{
	UTF::WithUTF16<int>(str, [&](const std::wstring_view str_w) {
		const auto hdc = GrpSurface_GDIText_Surface().dc;
		if(color) {
			SetColor(color.value());
		}
		RECT r = {
			.left = (rect.left + topleft_rel.x),
			.top = (rect.top + topleft_rel.y),
			.right = (rect.left + rect.w),
			.bottom = (rect.top + rect.h),
		};
		constexpr UINT flags = (DT_LEFT | DT_TOP | DT_SINGLELINE);
		return DrawTextW(hdc, str_w.data(), str_w.size(), &r, flags);
	});
}

bool TEXTRENDER_GDI::Wipe()
{
	if(
		!bounds ||
		(bounds.w > (std::numeric_limits<int32_t>::max)()) ||
		(bounds.h > (std::numeric_limits<int32_t>::max)())
	) {
		assert(!"Invalid size for blank surface");
		return false;
	}
	const auto w = static_cast<int32_t>(bounds.w);
	const auto h = static_cast<int32_t>(bounds.h);
	return (
		GrpSurface_GDIText_Create(w, h, { 0x00, 0x00, 0x00 }) &&
		TEXTRENDER_PACKED::Wipe()
	);
}

std::optional<TEXTRENDER_GDI_SESSION> TEXTRENDER_GDI::PreparePrerender(
	TEXTRENDER_RECT_ID rect_id
)
{
	const auto& surf = GrpSurface_GDIText_Surface();
	if((surf.size != bounds) && !Wipe()) {
		return std::nullopt;
	}
	assert(rect_id < rects.size());
	return TEXTRENDER_GDI_SESSION{ rects[rect_id].rect };
}

void TEXTRENDER_GDI::WipeBeforeNextRender()
{
	TEXTRENDER_PACKED::Wipe();

	// This also skips the needless creation of an uninitialized surface
	// during DirectDraw's init function.
	GrpSurface_GDIText_Surface().size = { 0, 0 };
}

PIXEL_SIZE TEXTRENDER_GDI::TextExtent(FONT_ID font, Narrow::string_view str)
{
	return TextGDIExtent(Fonts.ForID(font), str);
}

bool TEXTRENDER_GDI::Blit(
	WINDOW_POINT dst,
	TEXTRENDER_RECT_ID rect_id,
	std::optional<PIXEL_LTWH> subrect
)
{
	const PIXEL_LTRB rect = Subrect(rect_id, subrect);
	return GrpSurface_Blit(dst, SURFACE_ID::TEXT, rect);
}

void TextBackend_Cleanup(void)
{
	Fonts.Cleanup();
}
