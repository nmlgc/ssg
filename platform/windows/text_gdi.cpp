/*
 *   Text rendering via GDI
 *
 */

#define WIN32_LEAN_AND_MEAN

#include "platform/windows/text_gdi.h"
#include <assert.h>
#include <malloc.h>

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

void TEXTRENDER_GDI_SESSION_BASE::Put(
	const PIXEL_POINT& topleft_rel,
	Narrow::string_view str,
	std::optional<RGBA> color
)
{
	if(str.size() == 0) {
		return;
	}

	auto* str_w = static_cast<wchar_t *>(_malloca(
		str.size() * sizeof(wchar_t)
	));
	if(!str_w) {
		return;
	}

	// Try UTF-8
	auto len_w = MultiByteToWideChar(
		CP_UTF8, MB_ERR_INVALID_CHARS, str.data(), str.size(), str_w, str.size()
	);
	if((len_w == 0) && (GetLastError() == ERROR_NO_UNICODE_TRANSLATION)) {
		// Fall back on Shift-JIS
		len_w = MultiByteToWideChar(
			932, MB_PRECOMPOSED, str.data(), str.size(), str_w, str.size()
		);
		assert(len_w > 0);
	}

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
	DrawTextW(hdc, str_w, len_w, &r, flags);
	_freea(str_w);
}
