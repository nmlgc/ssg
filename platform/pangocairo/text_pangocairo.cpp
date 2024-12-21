/*
 *   Text rendering via Pango/Cairo
 *
 */

#include "platform/text_backend.h"
#include <pango/pangocairo.h>

constexpr auto FORMAT = CAIRO_FORMAT_ARGB32;
extern const ENUMARRAY<const char *, FONT_ID> FontSpecs;

struct PANGOCAIRO_STATE {
	cairo_surface_t *surf = nullptr;
	cairo_t *cr = nullptr;
	PangoLayout *layout = nullptr;

	PANGOCAIRO_STATE(void) {
	}
	PANGOCAIRO_STATE(cairo_surface_t *);
	PANGOCAIRO_STATE(PANGOCAIRO_STATE&&) noexcept;
	~PANGOCAIRO_STATE(void);

	explicit operator bool(void) const;
	PANGOCAIRO_STATE& operator=(PANGOCAIRO_STATE&&) noexcept;

	void SetText(Narrow::string_view);
	PIXEL_SIZE Extent(PangoFontDescription *, Narrow::string_view);
};

// State
// -----

TEXTRENDER_PANGOCAIRO TextObj;

// The single surface for the largest rectangle, enlarged by Session() as
// necessary.
PANGOCAIRO_STATE State;

static class {
	ENUMARRAY<PangoFontDescription *, FONT_ID> arr;

public:
	PangoFontDescription *ForID(FONT_ID font)
	{
		if(!arr[font]) {
			arr[font] = pango_font_description_from_string(FontSpecs[font]);
		}
		return arr[font];
	}

	void Cleanup(void)
	{
		for(auto& font : arr) {
			pango_font_description_free(font);
			font = nullptr;
		}
	}
} Fonts;
// -----

PANGOCAIRO_STATE::PANGOCAIRO_STATE(cairo_surface_t *surf) : surf(surf)
{
	if(!surf) {
		return;
	}
	cr = cairo_create(surf);
	assert(cr != nullptr); // Documentation says this will never fail
	layout = pango_cairo_create_layout(cr);
}

PANGOCAIRO_STATE::PANGOCAIRO_STATE(PANGOCAIRO_STATE&& other) noexcept
{
	*this = std::move(other);
}

PANGOCAIRO_STATE::~PANGOCAIRO_STATE(void)
{
	// Actually throws a warning if we pass a `nullptr`!
	if(layout) {
		g_object_unref(layout);
	}
	if(cr) {
		cairo_destroy(cr);
	}
	if(surf) {
		cairo_surface_destroy(surf);
	}
}

PANGOCAIRO_STATE::operator bool(void) const
{
	return (cr && layout);
}

PANGOCAIRO_STATE& PANGOCAIRO_STATE::operator=(PANGOCAIRO_STATE&& other) noexcept
{
	surf = std::exchange(other.surf, nullptr);
	cr = std::exchange(other.cr, nullptr);
	layout = std::exchange(other.layout, nullptr);
	return *this;
};

void PANGOCAIRO_STATE::SetText(Narrow::string_view str)
{
	const auto* in_buf = str.data();
	const auto in_size = str.size();
	if(g_utf8_validate_len(in_buf, in_size, nullptr)) {
		pango_layout_set_text(layout, in_buf, in_size);
	} else {
		gsize out_size;
		GError *error = nullptr;
		auto *out_buf = g_convert(
			in_buf, in_size, "UTF-8", "CP932", nullptr, &out_size, &error
		);
		pango_layout_set_text(layout, out_buf, out_size);
		g_free(out_buf);
	}
}

PIXEL_SIZE PANGOCAIRO_STATE::Extent(
	PangoFontDescription *font, Narrow::string_view str
)
{
	PIXEL_SIZE ret = { 0, 0 };
	if(font) {
		pango_layout_set_font_description(layout, font);
	}
	SetText(str);
	pango_layout_get_pixel_size(layout, &ret.w, &ret.h);
	return ret;
}

uint32_t& TEXTRENDER_PANGOCAIRO_SESSION::PIXELACCESS::PixelAt(
	const PIXEL_POINT& xy_rel
)
{
	return (std::bit_cast<uint32_t *>(buf + (stride * xy_rel.y)))[xy_rel.x];
}

uint32_t TEXTRENDER_PANGOCAIRO_SESSION::PIXELACCESS::GetRaw(
	const PIXEL_POINT& xy_rel
)
{
	return PixelAt(xy_rel);
}

void TEXTRENDER_PANGOCAIRO_SESSION::PIXELACCESS::SetRaw(
	const PIXEL_POINT& xy_rel, uint32_t color
)
{
	PixelAt(xy_rel) = color;
}

RGB TEXTRENDER_PANGOCAIRO_SESSION::PIXELACCESS::Get(const PIXEL_POINT& xy_rel)
{
	const auto ret = GetRaw(xy_rel);
	const uint8_t b = (ret >>  0);
	const uint8_t g = (ret >>  8);
	const uint8_t r = (ret >> 16);
	return RGB{ .r = r, .g = g, .b = b };
}

void TEXTRENDER_PANGOCAIRO_SESSION::PIXELACCESS::Set(
	const PIXEL_POINT& xy_rel, const RGB c
)
{
	SetRaw(xy_rel, (0xFF000000u | (c.r << 16u) | (c.g << 8u) | c.b));
}

TEXTRENDER_PANGOCAIRO_SESSION::PIXELACCESS::PIXELACCESS(void)
{
	cairo_surface_flush(State.surf);
	buf = cairo_image_surface_get_data(State.surf);
	stride = cairo_image_surface_get_stride(State.surf);
}

TEXTRENDER_PANGOCAIRO_SESSION::PIXELACCESS::~PIXELACCESS(void)
{
	cairo_surface_mark_dirty(State.surf);
}

TEXTRENDER_PANGOCAIRO_SESSION::TEXTRENDER_PANGOCAIRO_SESSION(
	const PIXEL_LTWH rect
) : tex_origin(rect.left, rect.top), size(rect.w, rect.h)
{
}

TEXTRENDER_PANGOCAIRO_SESSION::~TEXTRENDER_PANGOCAIRO_SESSION()
{
	cairo_surface_flush(State.surf);
	const auto *buf = cairo_image_surface_get_data(State.surf);
	const auto stride = cairo_image_surface_get_stride(State.surf);
	const PIXEL_LTWH subrect = { tex_origin.x, tex_origin.y, size.w, size.h };
	const auto sid = SURFACE_ID::TEXT;
	GrpSurface_Update(
		sid, &subrect, { std::bit_cast<const std::byte *>(buf), stride }
	);
}

PIXEL_SIZE TEXTRENDER_PANGOCAIRO_SESSION::RectSize(void) const
{
	return size;
}

void TEXTRENDER_PANGOCAIRO_SESSION::SetFont(FONT_ID font)
{
	if(font_cur != font) {
		pango_layout_set_font_description(State.layout, Fonts.ForID(font));
		font_cur = font;
	}
}

void TEXTRENDER_PANGOCAIRO_SESSION::SetColor(const RGB& color)
{
	if(color_cur != color) {
		cairo_set_source_rgb(
			State.cr, (color.r / 255.0), (color.g / 255.0), (color.b / 255.0)
		);
		color_cur = color;
	}
}

PIXEL_SIZE TEXTRENDER_PANGOCAIRO_SESSION::Extent(Narrow::string_view str)
{
	return State.Extent(nullptr, str);
}

void TEXTRENDER_PANGOCAIRO_SESSION::Put(
	const PIXEL_POINT& topleft_rel,
	Narrow::string_view str,
	std::optional<RGB> color
)
{
	if(color) {
		SetColor(color.value());
	}
	State.SetText(str);
	cairo_move_to(State.cr, topleft_rel.x, topleft_rel.y);
	pango_cairo_show_layout(State.cr, State.layout);
}

std::optional<TEXTRENDER_PANGOCAIRO_SESSION> TEXTRENDER_PANGOCAIRO::Session(
	TEXTRENDER_RECT_ID rect_id
)
{
	assert(rect_id < rects.size());
	const auto& rect = rects[rect_id].rect;

	PIXEL_SIZE surf_size = { 0, 0 };
	if(State.surf) {
		surf_size.w = cairo_image_surface_get_width(State.surf);
		surf_size.h = cairo_image_surface_get_height(State.surf);
	}
	if((surf_size.w < rect.w) || (surf_size.h < rect.h)) {
		static_assert(std::same_as<decltype(bounds.w), int>);
		static_assert(std::same_as<decltype(bounds.h), int>);
		const auto w = std::max(surf_size.w, rect.w);
		const auto h = std::max(surf_size.h, rect.h);
		State = { cairo_image_surface_create(FORMAT, w, h) };
	} else if(State) {
		cairo_save(State.cr);
		cairo_set_operator(State.cr, CAIRO_OPERATOR_CLEAR);
		cairo_rectangle(State.cr, 0, 0, rect.w, rect.h);
		cairo_fill(State.cr);
		cairo_restore(State.cr);
	}
	if(!State) {
		return std::nullopt;
	}

	if(GrpSurface_Size(SURFACE_ID::TEXT) != bounds) {
		TEXTRENDER_PACKED::Wipe();
		if(!GrpSurface_CreateUninitialized(SURFACE_ID::TEXT, bounds)) {
			return std::nullopt;
		}
	}
	return std::make_optional<TEXTRENDER_PANGOCAIRO_SESSION>(rect);
}

void TEXTRENDER_PANGOCAIRO::WipeBeforeNextRender()
{
	TEXTRENDER_PACKED::Wipe();
}

PIXEL_SIZE TEXTRENDER_PANGOCAIRO::TextExtent(
	FONT_ID font, Narrow::string_view str
)
{
	// Luckily, Pango calculates extents just fine on 0×0 surfaces.
	if(!State) {
		State = { cairo_image_surface_create(FORMAT, 0, 0) };
	}
	return State.Extent(Fonts.ForID(font), str);
}

void TextBackend_Cleanup(void)
{
	State = {};
	Fonts.Cleanup();
}
