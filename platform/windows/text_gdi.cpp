/*
 *   Text rendering via GDI
 *
 */

#include "platform/windows/text_gdi.h"
#include "platform/windows/utf.h"

PIXEL_SIZE TextGDIExtent(
	HDC hdc, std::optional<HFONT> font, Narrow::string_view str
)
{
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

TEXTRENDER_GDI_SESSION_BASE::TEXTRENDER_GDI_SESSION_BASE(
	const PIXEL_LTWH& rect, HDC hdc, const std::span<HFONT> fonts
) :
	rect(rect), hdc(hdc), fonts(fonts)
{
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

TEXTRENDER_GDI_SESSION_BASE::~TEXTRENDER_GDI_SESSION_BASE()
{
	if(font_initial) {
		SelectObject(hdc, font_initial.value());
	}
}

void TEXTRENDER_GDI_SESSION_BASE::SetFont(size_t id)
{
	auto font_prev = SelectObject(hdc, fonts[id]);
	if(!font_initial) {
		font_initial = font_prev;
	}
}

void TEXTRENDER_GDI_SESSION_BASE::SetColor(const RGBA& color)
{
	const COLORREF color_gdi = RGB(color.r, color.g, color.b);
	if(color_cur != color_gdi) {
		SetTextColor(hdc, color_gdi);
		color_cur = color_gdi;
	}
}

PIXEL_SIZE TEXTRENDER_GDI_SESSION_BASE::Extent(Narrow::string_view str)
{
	return TextGDIExtent(hdc, std::nullopt, str);
}

void TEXTRENDER_GDI_SESSION_BASE::Put(
	const PIXEL_POINT& topleft_rel,
	Narrow::string_view str,
	std::optional<RGBA> color
)
{
	UTF::WithUTF16<int>(str, [&](const std::wstring_view str_w) {
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
