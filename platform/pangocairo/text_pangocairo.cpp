/*
 *   Text rendering via Pango/Cairo
 *
 */

#include "platform/text_backend.h"
#include "game/defer.h"
#include <fontconfig/fontconfig.h>
#include <pango/pangocairo.h>

constexpr auto FORMAT = CAIRO_FORMAT_ARGB32;
extern const ENUMARRAY<const char *, FONT_ID> FontSpecs;

struct PANGOCAIRO_FONT {
	PangoFontDescription *desc = nullptr;
	cairo_hint_metrics_t hint_metrics = CAIRO_HINT_METRICS_DEFAULT;
};

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

	void SetFont(const PANGOCAIRO_FONT *);
	void SetText(Narrow::string_view);
	PIXEL_SIZE Extent(const PANGOCAIRO_FONT *, Narrow::string_view);
};

// Pango's hinting (which is, *of course*, controlled by a property in Cairo
// that overrides Fontconfig and can't be overridden itself) renders MS Gothic
// and IPAMonaGothic bitmaps 1 pixel lower than GDI… *except* for
//
// • MS Gothic at 10px and 16px, and
// • IPAMonaGothic at 10px,
//
// which happen to render pixel-perfectly. To check whether the game is
// actually using the optionally installable MS Gothic, we have to run Pango's
// font substitution, but the mere call to pango_font_map_load_font() already
// irrevocably applies Cairo's default metric hinting value to the respective
// entry in the Pango context's font map. Applying the Fontconfig substitutions
// is all we want here anyway.
bool MetricHintingNeededFor(PangoFontDescription *desc)
{
	const auto *family_in = pango_font_description_get_family(desc);
	const auto size = pango_font_description_get_size(desc);
	if(!family_in) {
		return false;
	}
	FcPattern *pat_in = FcPatternCreate();
	if(!pat_in) {
		return false;
	}
	defer(FcPatternDestroy(pat_in));

	// PangoFc only exposes a conversion from `PangoFontDescription` to
	// `FcPattern`, but *of course* not the other way around...
	auto **families = g_strsplit(family_in, ",", -1);
	for(auto i = 0; families[i] != nullptr; i++) {
		auto *family = std::bit_cast<FcChar8 *>(families[i]);
		FcPatternAddString(pat_in, FC_FAMILY, family);
	}
	g_strfreev(families);

	FcConfigSubstitute(nullptr, pat_in, FcMatchPattern);
	FcDefaultSubstitute(pat_in);

	FcResult result;
	auto* pat_out = FcFontMatch(nullptr, pat_in, &result);
	defer(FcPatternDestroy(pat_out));

	char *family_out = nullptr;
	const auto matched = FcPatternGetString(
		pat_out, FC_FAMILY, 0, std::bit_cast<FcChar8 **>(&family_out)
	);
	if(matched != FcResultMatch) {
		return false;
	}

	const auto is_ms_gothic = !strcmp(family_out, "MS Gothic");
	const auto is_ipamona_gothic = !strcmp(family_out, "IPAMonaGothic");
	switch(size) {
	case (10 * PANGO_SCALE):	return (is_ms_gothic || is_ipamona_gothic);
	case (16 * PANGO_SCALE):	return is_ms_gothic;
	}
	return false;
}

// State
// -----

TEXTRENDER TextObj;

// The single surface for the largest rectangle, enlarged by Session() as
// necessary.
PANGOCAIRO_STATE State;

static class {
	ENUMARRAY<PANGOCAIRO_FONT, FONT_ID> arr;

public:
	const PANGOCAIRO_FONT& ForID(FONT_ID font)
	{
		if(!arr[font].desc) {
			arr[font].desc = pango_font_description_from_string(
				FontSpecs[font]
			);
			arr[font].hint_metrics = (MetricHintingNeededFor(arr[font].desc)
				? CAIRO_HINT_METRICS_ON
				: CAIRO_HINT_METRICS_OFF
			);
		}
		return arr[font];
	}

	void Cleanup(void)
	{
		for(auto& font : arr) {
			pango_font_description_free(font.desc);
			font.desc = nullptr;
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

void PANGOCAIRO_STATE::SetFont(const PANGOCAIRO_FONT *font)
{
	if(!font) {
		return;
	}
	auto *opts = cairo_font_options_create();
	assert(opts != nullptr); // Documentation says this will never fail
	cairo_get_font_options(cr, opts);

	// Each PangoLayout has its own copy of Cairo's font options, which we (of
	// *course*) can't get to from the outside, forcing us to recreate the
	// layout...
	if(cairo_font_options_get_hint_metrics(opts) != font->hint_metrics) {
		cairo_font_options_set_hint_metrics(opts, font->hint_metrics);
		cairo_set_font_options(cr, opts);
		g_object_unref(layout);
		layout = pango_cairo_create_layout(cr);
	}
	cairo_font_options_destroy(opts);
	pango_layout_set_font_description(layout, font->desc);
}

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
	const PANGOCAIRO_FONT *font, Narrow::string_view str
)
{
	PIXEL_SIZE ret = { 0, 0 };
	SetFont(font);
	SetText(str);
	pango_layout_get_pixel_size(layout, &ret.w, &ret.h);
	return ret;
}

uint32_t& TEXTRENDER_SESSION::PIXELACCESS::PixelAt(const PIXEL_POINT& xy_rel)
{
	return (std::bit_cast<uint32_t *>(buf + (stride * xy_rel.y)))[xy_rel.x];
}

uint32_t TEXTRENDER_SESSION::PIXELACCESS::GetRaw(const PIXEL_POINT& xy_rel)
{
	return PixelAt(xy_rel);
}

void TEXTRENDER_SESSION::PIXELACCESS::SetRaw(
	const PIXEL_POINT& xy_rel, uint32_t color
)
{
	PixelAt(xy_rel) = color;
}

RGB TEXTRENDER_SESSION::PIXELACCESS::Get(const PIXEL_POINT& xy_rel)
{
	const auto ret = GetRaw(xy_rel);
	const uint8_t b = (ret >>  0);
	const uint8_t g = (ret >>  8);
	const uint8_t r = (ret >> 16);
	return RGB{ .r = r, .g = g, .b = b };
}

void TEXTRENDER_SESSION::PIXELACCESS::Set(
	const PIXEL_POINT& xy_rel, const RGB c
)
{
	SetRaw(xy_rel, (0xFF000000u | (c.r << 16u) | (c.g << 8u) | c.b));
}

TEXTRENDER_SESSION::PIXELACCESS::PIXELACCESS(void)
{
	cairo_surface_flush(State.surf);
	buf = cairo_image_surface_get_data(State.surf);
	stride = cairo_image_surface_get_stride(State.surf);
}

TEXTRENDER_SESSION::PIXELACCESS::~PIXELACCESS(void)
{
	cairo_surface_mark_dirty(State.surf);
}

TEXTRENDER_SESSION::TEXTRENDER_SESSION(const PIXEL_LTWH rect) :
	tex_origin(rect.left, rect.top), size(rect.w, rect.h)
{
}

TEXTRENDER_SESSION::~TEXTRENDER_SESSION()
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

PIXEL_SIZE TEXTRENDER_SESSION::RectSize(void) const
{
	return size;
}

void TEXTRENDER_SESSION::SetFont(FONT_ID font)
{
	if(font_cur != font) {
		State.SetFont(&Fonts.ForID(font));
		font_cur = font;
	}
}

void TEXTRENDER_SESSION::SetColor(const RGB& color)
{
	if(color_cur != color) {
		cairo_set_source_rgb(
			State.cr, (color.r / 255.0), (color.g / 255.0), (color.b / 255.0)
		);
		color_cur = color;
	}
}

PIXEL_SIZE TEXTRENDER_SESSION::Extent(Narrow::string_view str)
{
	return State.Extent(nullptr, str);
}

void TEXTRENDER_SESSION::Put(
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

std::optional<TEXTRENDER_SESSION> TEXTRENDER::Session(
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
	return std::make_optional<TEXTRENDER_SESSION>(rect);
}

void TEXTRENDER::WipeBeforeNextRender()
{
	TEXTRENDER_PACKED::Wipe();
}

PIXEL_SIZE TEXTRENDER::TextExtent(FONT_ID font, Narrow::string_view str)
{
	// Luckily, Pango calculates extents just fine on 0×0 surfaces.
	if(!State) {
		State = { cairo_image_surface_create(FORMAT, 0, 0) };
	}
	return State.Extent(&Fonts.ForID(font), str);
}

void TextBackend_Cleanup(void)
{
	State = {};
	Fonts.Cleanup();
}
