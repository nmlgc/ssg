/*
 *   Graphics via SDL_Renderer
 *
 */

// SDL headers must come first to avoid import→#include bugs on Clang 19.
#include <SDL_mouse.h>
#include <SDL_render.h>

#include "platform/sdl/graphics_sdl.h"
#include "platform/sdl/log_sdl.h"
#include "platform/sdl/window_sdl.h"
#include "platform/window_backend.h"
#include "game/defer.h"
#include "game/enum_array.h"
#include "game/format_bmp.h"
#include "game/string_format.h"
#include "constants.h"

static constexpr auto LOG_CAT = SDL_LOG_CATEGORY_RENDER;

/// State
/// -----

PIXELFORMAT PreferredPixelFormat;
SDL_ScaleMode TextureScaleMode = SDL_ScaleModeNearest;

// Primary renderer
// ----------------
// Probably hardware-accelerated, but not necessarily.

SDL_Renderer *PrimaryRenderer = nullptr;
SDL_RendererInfo PrimaryInfo;

// Rendering target in framebuffer scale mode. Not to be confused with
// [SoftwareTexture], which is a streaming texture, not a render target.
SDL_Texture *PrimaryTexture = nullptr;

// Buffer for screenshots at non-native resolutions when using geometry
// scaling. Allocated on demand.
SDL_Surface *GeometryScaledBackbuffer = nullptr;
// ----------------

// Software renderer
// -----------------

SDL_Renderer *SoftwareRenderer = nullptr;

// Used as the destination for software rendering, and as temporary storage for
// screenshots at native resolution.
SDL_Surface *SoftwareSurface = nullptr;

// Hardware-accelerated copy of [SoftwareSurface], streamed to every frame.
// Not to be confused with [PrimaryTexture], which is a render target, not a
// streaming texture.
SDL_Texture *SoftwareTexture = nullptr;
// -----------------

SDL_Renderer *Renderer = nullptr;

// Storing their associated renderer (primary or software) in the user data.
ENUMARRAY<SDL_Texture *, SURFACE_ID> Textures;

GRAPHICS_GEOMETRY_SDL GrpGeomSDL;

static RGBA Col = { 0, 0, 0, 0xFF };
static SDL_BlendMode AlphaMode = SDL_BLENDMODE_NONE;
/// -----

// Compile-time index buffers
// --------------------------

using INDEX_TYPE = uint8_t;

constexpr int TriangleIndexCount(size_t vertex_count)
{
	return ((vertex_count - 3 + 1) * 3);
}

constinit const auto TRIANGLE_FAN = ([] {
	constexpr INDEX_TYPE max = GRP_TRIANGLES_MAX;
	std::array<INDEX_TYPE, TriangleIndexCount(max)> ret;
	auto ret_p = ret.begin();
	for(const auto& i : std::views::iota(0u, (max - 2u))) {
		*(ret_p++) = 0;
		*(ret_p++) = (i + 1);
		*(ret_p++) = (i + 2);
	}
	return ret;
})();

constinit const auto TRIANGLE_STRIP = ([] {
	constexpr INDEX_TYPE max = GRP_TRIANGLES_MAX;
	std::array<INDEX_TYPE, TriangleIndexCount(max)> ret;
	auto ret_p = ret.begin();
	for(const auto& i : std::views::iota(0u, (max - 2u))) {
		*(ret_p++) = (i + 0);
		*(ret_p++) = (i + 1);
		*(ret_p++) = (i + 2);
	}
	return ret;
})();

constinit const ENUMARRAY<
	std::span<const INDEX_TYPE>, TRIANGLE_PRIMITIVE
> INDICES = { TRIANGLE_FAN, TRIANGLE_STRIP };
// --------------------------

// Helpers
// -------

SDL_FPoint HelpFPointFrom(const WINDOW_POINT& p)
{
	return SDL_FPoint{ static_cast<float>(p.x), static_cast<float>(p.y) };
}

SDL_Rect HelpRectFrom(const PIXEL_LTWH& o) noexcept
{
	return SDL_Rect{ .x = o.left, .y = o.top, .w = o.w, .h = o.h };
}

SDL_Rect HelpRectFrom(const PIXEL_LTRB& o)
{
	return SDL_Rect{
		.x = o.left,
		.y = o.top,
		.w = (o.right - o.left),
		.h = (o.bottom - o.top),
	};
}

std::span<const SDL_FPoint> HelpFPointsFrom(VERTEX_XY_SPAN<> sp)
{
	using GT = decltype(sp)::value_type;
	static_assert(sizeof(SDL_FPoint) == sizeof(GT));
	static_assert(std::is_same_v<decltype(SDL_FPoint::x), decltype(GT::x)>);
	static_assert(std::is_same_v<decltype(SDL_FPoint::y), decltype(GT::y)>);
	return { reinterpret_cast<const SDL_FPoint *>(sp.data()), sp.size() };
}

std::span<const SDL_Color> HelpColorsFrom(VERTEX_RGBA_SPAN<> sp)
{
	using GT = decltype(sp)::value_type;
	static_assert(sizeof(SDL_Color) == sizeof(GT));
	static_assert(std::is_same_v<decltype(SDL_Color::r), decltype(GT::r)>);
	static_assert(std::is_same_v<decltype(SDL_Color::g), decltype(GT::g)>);
	static_assert(std::is_same_v<decltype(SDL_Color::b), decltype(GT::b)>);
	static_assert(std::is_same_v<decltype(SDL_Color::a), decltype(GT::a)>);
	return { reinterpret_cast<const SDL_Color *>(sp.data()), sp.size() };
}

SDL_Texture *TexturePostInit(SDL_Texture& tex)
{
	SDL_SetTextureUserData(&tex, Renderer);
	SDL_SetTextureScaleMode(&tex, TextureScaleMode);
	return &tex;
}

SDL_Texture *HelpCreateTexture(uint32_t format, int access, int w, int h)
{
	auto *ret = SDL_CreateTexture(Renderer, format, access, w, h);
	if(!ret) {
		return ret;
	}
	return TexturePostInit(*ret);
}

// In SDL 2, switching from a texture render target back to NULL not only
// resets the renderer's viewport, clipping rectangle, and scaling settings
// back to the last state they were in on the raw renderer, but also loses any
// of these state changes made while rendering to the texture. This forces us
// to manually back up at least the one thing we care about, i.e. the clipping
// rectangle. SDL 3 fixes this by storing this view state as a part of each
// render target.
class SDL2_RENDER_TARGET_QUIRK_WORKAROUND {
	SDL_Renderer& renderer;
	bool did_clip{};
	SDL_Rect clip{};

public:
	SDL2_RENDER_TARGET_QUIRK_WORKAROUND(SDL_Renderer& renderer) :
		renderer(renderer), did_clip(SDL_RenderIsClipEnabled(&renderer)) {
		if(did_clip) {
			SDL_RenderGetClipRect(&renderer, &clip);
		}
	}

	~SDL2_RENDER_TARGET_QUIRK_WORKAROUND() {
		if(did_clip) {
			SDL_RenderSetClipRect(&renderer, &clip);
		}
	}
};

template <typename T> [[nodiscard]] T *SafeDestroy(void Destroy(T *), T *v)
{
	if(v) {
		Destroy(v);
		v = nullptr;
	}
	return v;
}

int SetRenderTargetFor(const SDL_Renderer *renderer)
{
	if(renderer == SoftwareRenderer) {
		return SDL_SetRenderTarget(PrimaryRenderer, nullptr);
	} else if((renderer == PrimaryRenderer) && PrimaryTexture) {
		return SDL_SetRenderTarget(PrimaryRenderer, PrimaryTexture);
	}
	return 0;
}

void SwitchActiveRenderer(SDL_Renderer *new_renderer)
{
	for(auto& tex : Textures) {
		if(tex && (SDL_GetTextureUserData(tex) == Renderer)) {
			tex = SafeDestroy(SDL_DestroyTexture, tex);
		}
	}
	SetRenderTargetFor(new_renderer);
	Renderer = new_renderer;
}

bool PixelFormatSupported(uint32_t fmt)
{
	// Both screenshots and the Pango/Cairo text backend currently expect ARGB
	// order.
	if(SDL_PIXELORDER(fmt) == SDL_PACKEDORDER_ABGR) {
		return false;
	}
	return (
		(SDL_ISPIXELFORMAT_PACKED(fmt) || SDL_ISPIXELFORMAT_INDEXED(fmt)) &&
		BITDEPTHS::find(SDL_BITSPERPIXEL(fmt))
	);
}

bool HelpSwitchFullscreen(
	const GRAPHICS_FULLSCREEN_FLAGS& fs_prev,
	const GRAPHICS_FULLSCREEN_FLAGS& fs_new
)
{
	auto *window = WndBackend_SDL();

	if(!fs_prev.fullscreen && fs_new.fullscreen) {
		TopleftBeforeFullscreen = HelpGetWindowPosition(window);
	}

	if(SDL_SetWindowFullscreen(window, HelpFullscreenFlag(fs_new)) != 0) {
		Log_Fail(LOG_CAT, "Error changing display mode");
		return false;
	}

	// If we come out of fullscreen mode, recenter the window.
	if(fs_prev.fullscreen && !fs_new.fullscreen) {
		const auto disp_i = std::max(SDL_GetWindowDisplayIndex(window), 0);
		const auto center = SDL_WINDOWPOS_CENTERED_DISPLAY(disp_i);
		SDL_SetWindowPosition(window, center, center);
	}
	return true;
}
// -------

// Pretty API version strings
// --------------------------

constexpr std::pair<std::u8string_view, std::u8string_view> API_NICE[] = {
	{ u8"direct3d", u8"Direct3D 9" },
	{ u8"direct3d11", u8"Direct3D 11" },
	{ u8"direct3d12", u8"Direct3D 12" },
	{ u8"software", u8"Software" },
};

namespace APIVersions {
constexpr const char FMT[] = "%s %d.%d";
constexpr size_t PRETTY_SIZE_MAX = 9;
constexpr size_t FMT_SIZE = ((sizeof(FMT) - 1) + (2 * STRING_NUM_CAP<int>));

struct VERSION {
	std::u8string_view name_sdl;
	const char *name_pretty;
	void (*update)(VERSION& self);
	char8_t buf[FMT_SIZE + PRETTY_SIZE_MAX + 1];
};

void UpdateOpenGL(VERSION& self)
{
	int major = 0;
	int minor = 0;
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);

	auto* buf = reinterpret_cast<char *>(self.buf);
	sprintf(&buf[0], &FMT[0], self.name_pretty, major, minor);
}

#define TARGET_(pretty, maj, min) pretty " ~" #maj "." #min
#define TARGET(pretty, maj, min) TARGET_(pretty, maj, min)

constinit VERSION Versions[] = {
	{ u8"opengl", "OpenGL", UpdateOpenGL, TARGET(
		u8"OpenGL", OPENGL_TARGET_CORE_MAJ, OPENGL_TARGET_CORE_MIN
	) },
	{ u8"opengles2", "OpenGL ES", UpdateOpenGL, TARGET(
		u8"OpenGL ES", 2, OPENGL_TARGET_ES2_MIN
	) },
};

#undef TARGET
#undef TARGET_

void Update(int id)
{
	const auto name = WndBackend_SDLRendererName(id);
	auto *version = std::ranges::find(Versions, name, &VERSION::name_sdl);
	if(version == std::end(Versions)) {
		return;
	}
	version->update(*version);
}
} // namespace APIVersions
// --------------------------

/// Enumeration and pre-initialization queries
/// ------------------------------------------

bool GrpBackend_Enum(void)
{
	// Any SDL-specific initialization was already done as part of
	// SDL_Init(SDL_INIT_VIDEO).
	return true;
}

uint8_t GrpBackend_DeviceCount(void) { return 0; }
Any::string_view GrpBackend_DeviceName(uint8_t) { return {}; }

int8_t GrpBackend_APICount(void)
{
	return SDL_GetNumRenderDrivers();
}

std::u8string_view GrpBackend_APIName(int8_t id)
{
	const auto ret = WndBackend_SDLRendererName(id);
	for(const auto& nice : API_NICE) {
		if(nice.first == ret) {
			return nice.second;
		}
	}
	for(const auto& nice : APIVersions::Versions) {
		if(nice.name_sdl == ret) {
			return &nice.buf[0];
		}
	}
	return ret;
}

PIXEL_SIZE GrpBackend_DisplaySize(bool fullscreen)
{
	SDL_Rect rect{};
	SDL_DisplayMode display_mode{};
	auto *window = WndBackend_SDL();
	const auto display_i = (
		window ? std::max(SDL_GetWindowDisplayIndex(window), 0) : 0
	);
	const auto ret = (fullscreen
		? SDL_GetDesktopDisplayMode(display_i, &display_mode)
		: SDL_GetDisplayUsableBounds(display_i, &rect)
	);
	if(ret != 0) {
		Log_Fail(LOG_CAT, "Error retrieving display size");
		return GRP_RES;
	}
	if(fullscreen) {
		return { .w = display_mode.w, .h = display_mode.h };
	}
	return { .w = rect.w, .h = rect.h };
}
/// ------------------------------------------

/// Initialization and cleanup
/// --------------------------

bool DestroySoftwareRenderer(void)
{
	SoftwareRenderer = SafeDestroy(SDL_DestroyRenderer, SoftwareRenderer);
	SoftwareTexture = SafeDestroy(SDL_DestroyTexture, SoftwareTexture);
	return false;
}

void PrimaryCleanup(void)
{
	for(auto& tex : Textures) {
		tex = SafeDestroy(SDL_DestroyTexture, tex);
	}
	PrimaryTexture = SafeDestroy(SDL_DestroyTexture, PrimaryTexture);
	PrimaryRenderer = SafeDestroy(SDL_DestroyRenderer, PrimaryRenderer);
	Renderer = nullptr;
	WndBackend_Cleanup();
}

// Returns the new `SCALE_GEOMETRY` flag.
bool PrimarySetScale(bool geometry, const WINDOW_SIZE& scaled_res)
{
	const auto set_geometry = [] {
		if(PrimaryTexture) {
			const auto wa = SDL2_RENDER_TARGET_QUIRK_WORKAROUND{
				*PrimaryRenderer
			};
			PrimaryTexture = SafeDestroy(SDL_DestroyTexture, PrimaryTexture);
			SDL_RenderSetLogicalSize(PrimaryRenderer, GRP_RES.w, GRP_RES.h);
			// [wa] must drop after SDL_RenderSetLogicalSize()!
		} else {
			SDL_RenderSetLogicalSize(PrimaryRenderer, GRP_RES.w, GRP_RES.h);
		}
		return true;
	};

	// Update texture filters
	// ----------------------
	if((scaled_res.w % GRP_RES.w) || (scaled_res.h % GRP_RES.h)) {
		TextureScaleMode = SDL_ScaleModeBest;
	} else {
		TextureScaleMode = SDL_ScaleModeNearest;
	}
	for(auto& tex : Textures) {
		if(tex) {
			SDL_SetTextureScaleMode(tex, TextureScaleMode);
		}
	}
	if(SoftwareTexture) {
		SDL_SetTextureScaleMode(SoftwareTexture, TextureScaleMode);
	}
	if(PrimaryTexture) {
		SDL_SetTextureScaleMode(PrimaryTexture, TextureScaleMode);
	}
	// ----------------------

	GeometryScaledBackbuffer = SafeDestroy(
		SDL_FreeSurface, GeometryScaledBackbuffer
	);
	if(geometry || (scaled_res == GRP_RES)) {
		set_geometry();
		return geometry; // Don't unset the user's choice on 1× scaling!
	}

	assert(!geometry);
	if(!SDL_RenderTargetSupported(PrimaryRenderer)) {
		Log_Fail(LOG_CAT, "Render API does not support render targets");
		return set_geometry();
	}

	// Prepare the primary renderer for blitting the primary texture:
	// • Ensure the correct logical size
	// • Move the renderer's clipping rectangle onto whatever new renderer we
	//   target (won't be needed in SDL 3)
	// • Stop clipping on the primary renderer!!! Its clipping region is going
	//   to apply to the [PrimaryTexture] blit going forward!!!
	const auto wa = SDL2_RENDER_TARGET_QUIRK_WORKAROUND{ *Renderer };
	SDL_SetRenderTarget(PrimaryRenderer, nullptr);
	SDL_RenderSetClipRect(PrimaryRenderer, nullptr);
	SDL_RenderSetLogicalSize(PrimaryRenderer, scaled_res.w, scaled_res.h);

	if(!PrimaryTexture) {
		assert(SoftwareSurface);
		const auto format = SoftwareSurface->format->format;
		const auto& res = GRP_RES;
		PrimaryTexture = SDL_CreateTexture(
			PrimaryRenderer, format, SDL_TEXTUREACCESS_TARGET, res.w, res.h
		);
		if(!PrimaryTexture) {
			Log_Fail(LOG_CAT, "Error creating native resolution texture");
			return set_geometry();
		}
		SDL_SetTextureScaleMode(PrimaryTexture, TextureScaleMode);
	}

	// We might be software-rendering.
	if(SetRenderTargetFor(Renderer) != 0) {
		Log_Fail(LOG_CAT, "Error setting texture as render target");
		return set_geometry();
	}
	return geometry;
}

// Re-centers the window to remain fully on-screen after changing the
// windowed-mode scale factor
PIXEL_POINT RepositionAfterScale(
	const PIXEL_POINT& topleft_prev,
	const WINDOW_SIZE& res_prev,
	const WINDOW_SIZE& res_new
)
{
	auto *window = WndBackend_SDL();
	PIXEL_COORD border_left{};
	PIXEL_COORD border_top{};
	PIXEL_COORD border_right{};
	PIXEL_COORD border_bottom{};
	SDL_GetWindowBordersSize(
		window, &border_top, &border_left, &border_bottom, &border_right
	);

	const auto display_i = SDL_GetWindowDisplayIndex(window);
	if(display_i < 0) {
		return topleft_prev;
	}
	SDL_Rect display_r{};
	if(SDL_GetDisplayUsableBounds(display_i, &display_r) != 0) {
		return topleft_prev;
	}
	display_r.x += border_left;
	display_r.y += border_top;
	display_r.w -= (border_left + border_right);
	display_r.h -= (border_top + border_bottom);

	// The window might have been moved to a display with a resolution
	// smaller than [res_new].
	const auto max_left = std::max(
		display_r.x, (display_r.x + display_r.w - res_new.w)
	);
	const auto max_top = std::max(
		display_r.y, (display_r.y + display_r.h - res_new.h)
	);

	auto topleft = ((topleft_prev + (res_prev / 2)) - (res_new / 2));
	topleft.x = std::clamp(topleft.x, display_r.x, max_left);
	topleft.y = std::clamp(topleft.y, display_r.y, max_top);
	SDL_SetWindowPosition(window, topleft.x, topleft.y);
	return topleft;
}

void PrimarySetBorderlessFullscreenFit(
	GRAPHICS_PARAMS params, const WINDOW_SIZE& scaled_res
)
{
	using FIT = GRAPHICS_FULLSCREEN_FIT;
	const auto fs = params.FullscreenFlags();

	// Yet another state backup dance that won't be necessary in SDL 3...
	const auto wa = SDL2_RENDER_TARGET_QUIRK_WORKAROUND{ *PrimaryRenderer };

	auto *target = SDL_GetRenderTarget(PrimaryRenderer);
	SDL_SetRenderTarget(PrimaryRenderer, nullptr);

	if(fs.fullscreen && !fs.exclusive) {
		SDL_RenderSetIntegerScale(
			PrimaryRenderer, ((fs.fit == FIT::INTEGER) ? SDL_TRUE : SDL_FALSE)
		);

		// SDL_RenderSetLogicalSize() enforces the aspect ratio, which we
		// explicitly don't want in stretch mode.
		if(params.ScaleGeometry() && (fs.fit == FIT::STRETCH)) {
			const auto scale_w = (static_cast<float>(scaled_res.w) / GRP_RES.w);
			const auto scale_h = (static_cast<float>(scaled_res.h) / GRP_RES.h);
			SDL_RenderSetScale(PrimaryRenderer, scale_w, scale_h);
			SDL_RenderSetViewport(PrimaryRenderer, nullptr);
		}
	} else {
		SDL_RenderSetIntegerScale(PrimaryRenderer, SDL_FALSE);
	}
	SDL_SetRenderTarget(PrimaryRenderer, target);
}

std::optional<GRAPHICS_INIT_RESULT> PrimaryInitFull(GRAPHICS_PARAMS params)
{
	auto *window = WndBackend_SDLCreate(params);
	auto *renderer = SDL_CreateRenderer(
		window, params.api, SDL_RENDERER_ACCELERATED
	);
	if(!renderer) {
		const auto api_name = GrpBackend_APIName(params.api);
		const auto* api = std::bit_cast<const char *>(api_name.data());
		SDL_LogCritical(
			LOG_CAT, "Error creating %s renderer: %s", api, SDL_GetError()
		);
		WndBackend_Cleanup();
		return std::nullopt;
	}
	SDL_GetRendererInfo(renderer, &PrimaryInfo);

	// Determine preferred texture formats.
	// SDL_GetWindowPixelFormat() is *not* a shortcut we could use for software
	// rendering mode. On my system, it always returns the 24-bit RGB888, which
	// we don't support.
	const auto formats = std::span(
		&PrimaryInfo.texture_formats[0], PrimaryInfo.num_texture_formats
	);
	const auto sdl_format = std::ranges::find_if(formats, PixelFormatSupported);
	if(sdl_format == formats.end()) {
		SDL_LogCritical(
			LOG_CAT,
			"The \"%s\" renderer does not support any of the game's supported software rendering pixel formats.",
			PrimaryInfo.name
		);
		return std::nullopt;
	}

	PrimaryRenderer = renderer;
	SwitchActiveRenderer(PrimaryRenderer);
	APIVersions::Update(params.api);

	const auto bpp = SDL_BITSPERPIXEL(*sdl_format);
	const auto pixel_format = BITDEPTHS::find(bpp).pixel_format();
	if(!pixel_format) {
		assert(!"The pixel format should always be valid here");
		std::unreachable();
	}
	// We don't overwrite [params.bitdepth] here to allow frictionless
	// switching between the old DirectDraw/Direct3D backend and this one.
	PreferredPixelFormat = pixel_format.value();

	// Ensure that the software surface uses the preferred format
	if(!SoftwareSurface || (SoftwareSurface->format->format != *sdl_format)) {
		SoftwareSurface = SafeDestroy(SDL_FreeSurface, SoftwareSurface);
		SoftwareSurface = SDL_CreateRGBSurfaceWithFormat(
			0, GRP_RES.w, GRP_RES.h, 0, *sdl_format
		);
		if(!SoftwareSurface) {
			Log_Fail(LOG_CAT, "Error creating surface for software rendering");
			return std::nullopt;
		}
	}

	const auto res_new = params.ScaledRes();
	const auto geometry = PrimarySetScale(params.ScaleGeometry(), res_new);
	params.SetFlag(GRAPHICS_PARAM_FLAGS::SCALE_GEOMETRY, geometry);
	PrimarySetBorderlessFullscreenFit(params, res_new);

	return GRAPHICS_INIT_RESULT{ .live = params, .reload_surfaces = true };
}

std::optional<GRAPHICS_INIT_RESULT> GrpBackend_Init(
	std::optional<const GRAPHICS_PARAMS> maybe_prev, GRAPHICS_PARAMS params
)
{
	const auto reinit_full = [](const GRAPHICS_PARAMS& params) {
		PrimaryCleanup();
		return PrimaryInitFull(params);
	};

	const auto fs_new = params.FullscreenFlags();

	// This is the only place that applies to both a full init and a partial
	// update later...
	SDL_ShowCursor(!(fs_new.fullscreen && fs_new.exclusive));

	if(!maybe_prev) {
		return PrimaryInitFull(params);
	}
	const auto& prev = maybe_prev.value();
	const auto fs_prev = prev.FullscreenFlags();

	// API changes need a complete reinit.
	if(prev.api != params.api) {
		return reinit_full(params);
	}

	// As do a few things when switching to exclusive fullscreen.
	if(fs_new.fullscreen && fs_new.exclusive) {
		// The Direct3D renderer can only launch into exclusive fullscreen on
		// the same display the window was spawned on, so let's just throw it
		// away.
		const auto name = WndBackend_SDLRendererName(params.api);
		if(!fs_prev.fullscreen && (name == u8"direct3d")) {
			return reinit_full(params);
		}

		// If the window started out as borderless and should later switch to
		// exclusive, SDL simply doesn't do the mode change:
		//
		// 	https://github.com/libsdl-org/SDL/issues/11047
		if(fs_prev.fullscreen && !fs_prev.exclusive) {
			return reinit_full(params);
		}
	}

	// The following parameters can be changed on the fly, but we don't want to
	// reflect modifications of any parameters we don't care about.
	GRAPHICS_INIT_RESULT ret = { .live = prev, .reload_surfaces = false };
	using F = GRAPHICS_PARAM_FLAGS;

	// Apply fullscreen changes first, as exclusive fullscreen affects the
	// display size calculated by ScaledRes().
	if(
		(fs_prev.fullscreen != fs_new.fullscreen) ||
		(fs_prev.exclusive != fs_new.exclusive)
	) {
		if(HelpSwitchFullscreen(fs_prev, fs_new)) {
			// If we clipped on the raw renderer, the clipping rectangle won't
			// match the current resolution anymore.
			SDL_RenderSetClipRect(PrimaryRenderer, nullptr);

			ret.live.SetFlag(F::FULLSCREEN, fs_new.fullscreen);
			ret.live.SetFlag(F::FULLSCREEN_EXCLUSIVE, fs_new.exclusive);
		}
	}

	auto *window = WndBackend_SDL();
	WINDOW_SIZE res_prev{};
	SDL_GetWindowSize(window, &res_prev.w, &res_prev.h);

	const auto res_new = params.ScaledRes();
	const bool res_changed = (res_prev != res_new);
	if(res_changed && !fs_new.fullscreen) {
		const auto wa = SDL2_RENDER_TARGET_QUIRK_WORKAROUND{ *PrimaryRenderer };
		PIXEL_POINT topleft{};
		SDL_GetWindowPosition(window, &topleft.x, &topleft.y);
		SDL_SetWindowSize(window, res_new.w, res_new.h);
		topleft = RepositionAfterScale(topleft, res_prev, res_new);
		ret.live.left = topleft.x;
		ret.live.top = topleft.y;
	} else {
		ret.live.left = params.left;
		ret.live.top = params.top;
	}

	// Should always be applied unconditionally so that the user can change
	// from the maximum scale value to 0, both of which result in the same
	// scaled resolution.
	ret.live.window_scale_4x = params.window_scale_4x;

	const auto geometry = PrimarySetScale(params.ScaleGeometry(), res_new);
	ret.live.SetFlag(F::SCALE_GEOMETRY, geometry);

	PrimarySetBorderlessFullscreenFit(params, res_new);
	ret.live.SetFlag(F::FULLSCREEN_FIT, std::to_underlying(fs_new.fit));

	return ret;
}

void GrpBackend_Cleanup(void)
{
	PrimaryCleanup();
	DestroySoftwareRenderer();
	SoftwareSurface = SafeDestroy(SDL_FreeSurface, SoftwareSurface);
}
/// --------------------------

/// General
/// -------

void GrpBackend_Clear(uint8_t, RGB col)
{
	SDL_SetRenderDrawColor(Renderer, col.r, col.g, col.b, 0xFF);
	SDL_RenderClear(Renderer);
}

void GrpBackend_SetClip(const WINDOW_LTRB& rect)
{
	if(!Renderer) {
		return;
	}
	const auto sdl_rect = HelpRectFrom(rect);
	SDL_RenderSetClipRect(Renderer, &sdl_rect);
}

PIXELFORMAT GrpBackend_PixelFormat(void)
{
	return PreferredPixelFormat;
}

void GrpBackend_PaletteGet(PALETTE& pal) {}
bool GrpBackend_PaletteSet(const PALETTE& pal) { return false; }

void MaybeTakeScreenshot(std::unique_ptr<FILE_STREAM_WRITE> stream)
{
	if(!stream) {
		return;
	}

	SDL_Surface* src = SoftwareSurface;
	bool fill_src_with_pixels_from_renderer = false;

	if(SoftwareRenderer) {
		// Software rendering is the ideal case for screenshots, because we
		// already have a system-memory surface we can save.
		fill_src_with_pixels_from_renderer = false;
	} else if(PrimaryTexture) {
		// Thankfully we can SDL_RenderReadPixels() from a render target to get
		// a screenshot at the native resolution. [SoftwareSurface] uses the
		// same size, so let's use it as temporary storage.
		fill_src_with_pixels_from_renderer = true;
	} else {
		// If we aren't scaling, we are lucky and can just do the same we're
		// doing in the hardware texture case.
		SDL_Rect unscaled_viewport{};
		float scale_x{}, scale_y{};
		SDL_RenderGetViewport(PrimaryRenderer, &unscaled_viewport);
		SDL_RenderGetScale(PrimaryRenderer, &scale_x, &scale_y);
		const PIXEL_SIZE scaled_size = {
			.w = static_cast<PIXEL_COORD>(unscaled_viewport.w * scale_x),
			.h = static_cast<PIXEL_COORD>(unscaled_viewport.h * scale_y),
		};
		if(scaled_size == GRP_RES) {
			fill_src_with_pixels_from_renderer = true;
		} else {
			// If we are, we must take a screenshot at the scaled
			// resolution.
			if(!GeometryScaledBackbuffer) {
				const auto format = SoftwareSurface->format->format;
				GeometryScaledBackbuffer = SDL_CreateRGBSurfaceWithFormat(
					0, scaled_size.w, scaled_size.h, 0, format
				);
			}
			if(!GeometryScaledBackbuffer) {
				Log_Fail(
					LOG_CAT, "Failed to create temporary storage for screenshot"
				);
				return;
			}
			src = GeometryScaledBackbuffer;
			fill_src_with_pixels_from_renderer = true;
		}
	}

	if(SDL_MUSTLOCK(src)) {
		SDL_LockSurface(src);
	}
	if(fill_src_with_pixels_from_renderer) {
		if(SDL_RenderReadPixels(
			PrimaryRenderer,
			nullptr,
			src->format->format,
			src->pixels,
			src->pitch
		) != 0) {
			Log_Fail(LOG_CAT, "Error taking screenshot");
			return;
		}
	}
	const auto bytes = static_cast<std::byte *>(src->pixels);

	// Negate the height so that we neither have to flip the pixels nor write
	// RWops for `FILE_STREAM_WRITE` in order to use SDL_SaveBMP_RW().
	const PIXEL_SIZE size = { .w = src->w, .h = -src->h };
	assert(src->format->palette == nullptr);

	BMPSave(
		stream.get(),
		size,
		1,
		src->format->BitsPerPixel,
		std::span<BGRA>(),
		std::span(bytes, (src->h * src->pitch))
	);
	if(SDL_MUSTLOCK(src)) {
		SDL_UnlockSurface(src);
	}
	stream.reset();
}

void GrpBackend_Flip(std::unique_ptr<FILE_STREAM_WRITE> screenshot_stream)
{
	MaybeTakeScreenshot(std::move(screenshot_stream));
	if(SoftwareRenderer) {
		SDL_RenderFlush(SoftwareRenderer);
		if(SDL_MUSTLOCK(SoftwareSurface)) {
			SDL_LockSurface(SoftwareSurface);
		}
		SDL_UpdateTexture(
			SoftwareTexture,
			nullptr,
			SoftwareSurface->pixels,
			SoftwareSurface->pitch
		);
		if(SDL_MUSTLOCK(SoftwareSurface)) {
			SDL_UnlockSurface(SoftwareSurface);
		}
		SDL_RenderCopy(PrimaryRenderer, SoftwareTexture, nullptr, nullptr);
	} else if(PrimaryTexture) {
		const auto wa = SDL2_RENDER_TARGET_QUIRK_WORKAROUND{ *PrimaryRenderer };
		SDL_SetRenderTarget(PrimaryRenderer, nullptr);

		// In borderless fullscreen mode, the scaled texture may not cover the
		// entire window. Technically, we only need to do this once for every
		// backbuffer after switching the fullscreen fit, but:
		// 1) SDL has no way of querying the length of the swapchain, and
		// 2) you are supposed to do this on every frame anyway, as a lot of
		//    GPUs can use clearing as a performance hint.
		// Let's measure the performance impact on windowed mode some other
		// time...
		GrpBackend_Clear(0, RGB{ 0, 0, 0 });

		SDL_RenderCopy(PrimaryRenderer, PrimaryTexture, nullptr, nullptr);
		SDL_SetRenderTarget(PrimaryRenderer, PrimaryTexture);
	}
	SDL_RenderPresent(PrimaryRenderer);
}
/// -------

/// Surfaces
/// --------

bool CreateTextureWithFormat(
	SURFACE_ID sid, uint32_t fmt, const PIXEL_SIZE& size
)
{
	auto& tex = Textures[sid];
	tex = SafeDestroy(SDL_DestroyTexture, tex);

	tex = HelpCreateTexture(fmt, SDL_TEXTUREACCESS_STREAMING, size.w, size.h);
	if(!tex) {
		Log_Fail(LOG_CAT, "Error creating blank texture");
		return false;
	}
	if(SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND) != 0) {
		Log_Fail(LOG_CAT, "Error enabling alpha blending for GDI text texture");
		return false;
	}
	return true;
}

bool GrpSurface_CreateUninitialized(SURFACE_ID sid, const PIXEL_SIZE& size)
{
	return CreateTextureWithFormat(sid, SoftwareSurface->format->format, size);
}

bool GrpSurface_Load(SURFACE_ID sid, BMP_OWNED&& bmp)
{
	auto& tex = Textures[sid];
	tex = SafeDestroy(SDL_DestroyTexture, tex);

	auto *rwops = SDL_RWFromMem(bmp.buffer.get(), bmp.buffer.size());
	auto *surf = SDL_LoadBMP_RW(rwops, 1);
	defer(SDL_FreeSurface(surf));
	std::ignore = std::move(bmp);

	if(surf->format->format == SDL_PIXELFORMAT_INDEX8) {
		// The transparent pixel is in the top-left corner.
		if(SDL_MUSTLOCK(surf)) {
			SDL_LockSurface(surf);
		}
		const auto key = static_cast<uint8_t *>(surf->pixels)[0];
		if(SDL_MUSTLOCK(surf)) {
			SDL_UnlockSurface(surf);
		}
		SDL_SetColorKey(surf, SDL_TRUE, key);
	}

	tex = SDL_CreateTextureFromSurface(Renderer, surf);
	if(!tex) {
		Log_Fail(LOG_CAT, "Error loading .BMP as texture");
		return false;
	}
	TexturePostInit(*tex);
	return true;
}

bool GrpSurface_PaletteApplyToBackend(SURFACE_ID) { return true; }

bool GrpSurface_Update(
	SURFACE_ID sid,
	const PIXEL_LTWH* subrect,
	std::tuple<const std::byte *, size_t> pixels
) noexcept
{
	const auto [buf, pitch] = pixels;
	if(pitch > std::numeric_limits<int>::max()) {
		SDL_LogCritical(
			LOG_CAT, "Pitch of %zu bytes does not fit into an integer.", pitch
		);
		return false;
	}

	auto *tex = Textures[sid];
	if(!subrect) {
		return (SDL_UpdateTexture(tex, nullptr, buf, pitch) == 0);
	}
	const auto rect = HelpRectFrom(*subrect);
	return (SDL_UpdateTexture(tex, &rect, buf, pitch) == 0);
}


PIXEL_SIZE GrpSurface_Size(SURFACE_ID sid)
{
	PIXEL_SIZE ret = { 0, 0 };
	auto *tex = Textures[sid];
	if(!tex) {
		return ret;
	}
	SDL_QueryTexture(tex, nullptr, nullptr, &ret.w, &ret.h);
	return ret;
}

bool GrpSurface_Blit(
	WINDOW_POINT topleft, SURFACE_ID sid, const PIXEL_LTRB& src
)
{
	const auto rect_src = HelpRectFrom(src);
	const SDL_Rect rect_dst = {
		.x = topleft.x, .y = topleft.y, .w = rect_src.w, .h = rect_src.h
	};
	return (SDL_RenderCopy(Renderer, Textures[sid], &rect_src, &rect_dst) == 0);
}

void GrpSurface_BlitOpaque(
	WINDOW_POINT topleft, SURFACE_ID sid, const PIXEL_LTRB& src
)
{
	auto *tex = Textures[sid];
	SDL_BlendMode prev{};
	SDL_GetTextureBlendMode(tex, &prev);
	SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_NONE);
	GrpSurface_Blit(topleft, sid, src);
	SDL_SetTextureBlendMode(tex, prev);
}

#ifdef WIN32
// Win32 GDI text rendering bridge
// -------------------------------

#include "platform/windows/surface_gdi.h"

// SDL textures only support transparency via alpha blending, and the only
// alpha-blended formats available on any SDL_Renderer backend in a Windows
// build of SDL 2.30.6 are 32-bit ones. GDI also exclusively uses the BGRX
// memory order for 32-bit bitmaps. Might as well limit the GDI code to that
// one specific format then.
static constexpr auto GDITEXT_BPP = 32;
static constexpr uint32_t GDITEXT_SDL_FORMAT = SDL_PIXELFORMAT_ARGB8888;

static SURFACE_GDI GrText;

static uint32_t GDIText_ColorKey;
static uint32_t GDIText_AlphaMask;

SURFACE_GDI& GrpSurface_GDIText_Surface(void) noexcept
{
	return GrText;
}

bool GrpSurface_GDIText_Create(int32_t w, int32_t h, RGB colorkey)
{
	GrText.Delete();

	const auto formats = std::span(
		&PrimaryInfo.texture_formats[0], PrimaryInfo.num_texture_formats
	);
	if(std::ranges::find(formats, GDITEXT_SDL_FORMAT) == formats.end()) {
		SDL_LogCritical(
			LOG_CAT,
			"Renderer \"%s\" does not support the BGRA8888 pixel format required for rendering text via GDI.",
			PrimaryInfo.name
		);
		return false;
	};

	auto *format_struct = SDL_AllocFormat(GDITEXT_SDL_FORMAT);
	if(!format_struct) {
		Log_Fail(
			LOG_CAT, "Error retrieving format structure for GDI text surface"
		);
		return false;
	}
	defer(SDL_FreeFormat(format_struct));

	// GDI always sets the "alpha channel" to 0. Removing it from the color key
	// as well saves an OR operation in the alpha fixing loop.
	GDIText_AlphaMask = format_struct->Amask;
	GDIText_ColorKey = (
		SDL_MapRGB(format_struct, colorkey.r, colorkey.g, colorkey.b) &
		 ~GDIText_AlphaMask
	);

	const BITMAPINFOHEADER bmi = {
		.biSize = sizeof(bmi),
		.biWidth = w,
		.biHeight = -h,
		.biPlanes = 1,
		.biBitCount = GDITEXT_BPP,
		.biCompression = BI_RGB,
	};
	const auto *bi = reinterpret_cast<const BITMAPINFO *>(&bmi);
	void *dib_bits = nullptr;
	GrText.img = CreateDIBSection(GrText.dc, bi, 0, &dib_bits, nullptr, 0);
	if(!GrText.img) {
		SDL_LogCritical(LOG_CAT, "%s", "Error creating GDI text surface");
		return false;
	}
	GrText.size = { w, h };
	GrText.stock_img = SelectObject(GrText.dc, GrText.img);
	return CreateTextureWithFormat(
		SURFACE_ID::TEXT, GDITEXT_SDL_FORMAT, { w, h }
	);
}

bool GrpSurface_GDIText_Update(const PIXEL_LTWH& r) noexcept
{
	DIBSECTION dib;
	if(!GetObject(GrText.img, sizeof(DIBSECTION), &dib)) {
		return false;
	}

	auto *pixels = (
		static_cast<std::byte *>(dib.dsBm.bmBits) +
		(r.top * dib.dsBm.bmWidthBytes) +
		(r.left * (dib.dsBmih.biBitCount / 8))
	);

	static_assert((GDITEXT_BPP == 32), "Only tested for 32-bit.");
	const auto w = static_cast<size_t>(r.w);
	const auto h = static_cast<size_t>(r.h);
	auto *row_p = pixels;
	for(const auto y : std::views::iota(0u, h)) {
		auto pixels_in_row = std::span(reinterpret_cast<uint32_t *>(row_p), w);
		for(auto& pixel : pixels_in_row) {
			if(pixel != GDIText_ColorKey) {
				pixel |= GDIText_AlphaMask;
			}
		}
		row_p += dib.dsBm.bmWidthBytes;
	};
	const auto pitch = static_cast<size_t>(dib.dsBm.bmWidthBytes);
	return GrpSurface_Update(SURFACE_ID::TEXT, &r, { pixels, pitch });
}
// -------------------------------
#endif
/// --------

/// Geometry
/// --------

void DrawGeometry(
	TRIANGLE_PRIMITIVE tp, VERTEX_XY_SPAN<> xys, VERTEX_RGBA_SPAN<> colors
)
{
	#pragma warning(suppress : 26494) // type.5
	SDL_FPoint sdl_vertices[GRP_TRIANGLES_MAX];

	const auto vertex_count = xys.size();
	const auto sdl_colors = HelpColorsFrom(colors);
	const auto indices = INDICES[tp];
	const auto index_count = TriangleIndexCount(vertex_count);
	assert(vertex_count <= std::size(sdl_vertices));
	assert(index_count <= indices.size());
	assert((colors.size() == 1) || (colors.size() == vertex_count));

	// Work around SDL's weird -0.5f offset...
	float offset_x, offset_y;
	SDL_RenderGetScale(Renderer, &offset_x, &offset_y);
	offset_x = (1.0f / (2.0f * offset_x));
	offset_y = (1.0f / (2.0f * offset_y));
	auto sdl = std::begin(sdl_vertices);
	for(const auto& game : xys) {
		*(sdl++) = { .x = (game.x + offset_x), .y = (game.y + offset_y) };
	}

	SDL_RenderGeometryRaw(
		Renderer,
		nullptr,
		&sdl_vertices[0].x,
		sizeof(SDL_FPoint),
		sdl_colors.data(),
		((sdl_colors.size() == 1) ? 0 : sizeof(SDL_Color)),
		nullptr,
		0,
		vertex_count,
		indices.data(),
		index_count,
		sizeof(INDEX_TYPE)
	);
}

static void DrawWithAlpha(auto func)
{
	SDL_SetRenderDrawBlendMode(Renderer, AlphaMode);
	SDL_SetRenderDrawColor(Renderer, Col.r, Col.g, Col.b, Col.a);
	func();
	SDL_SetRenderDrawColor(Renderer, Col.r, Col.g, Col.b, 0xFF);
	SDL_SetRenderDrawBlendMode(Renderer, SDL_BLENDMODE_NONE);
}

GRAPHICS_GEOMETRY_SDL *GrpGeom_Poly(void)
{
	return &GrpGeomSDL;
}

GRAPHICS_GEOMETRY_SDL *GrpGeom_FB(void) { return nullptr; }

void GRAPHICS_GEOMETRY_SDL::Lock(void) {}
void GRAPHICS_GEOMETRY_SDL::Unlock(void) {}

void GRAPHICS_GEOMETRY_SDL::SetColor(RGB216 col)
{
	const auto rgb = col.ToRGB();
	Col.r = rgb.r;
	Col.g = rgb.g;
	Col.b = rgb.b;
	SDL_SetRenderDrawColor(Renderer, Col.r, Col.g, Col.b, 0xFF);
}

void GRAPHICS_GEOMETRY_SDL::SetAlphaNorm(uint8_t a)
{
	Col.a = a;
	AlphaMode = SDL_BLENDMODE_BLEND;
}

void GRAPHICS_GEOMETRY_SDL::SetAlphaOne(void)
{
	Col.a = 0xFF;
	AlphaMode = SDL_BLENDMODE_ADD;
}

void GRAPHICS_GEOMETRY_SDL::DrawLine(int x1, int y1, int x2, int y2)
{
	SDL_RenderDrawLine(Renderer, x1, y1, x2, y2);
}

void GRAPHICS_GEOMETRY_SDL::DrawBox(int x1, int y1, int x2, int y2)
{
	const SDL_Rect rect = { .x = x1, .y = y1, .w = (x2 - x1), .h = (y2 - y1) };
	SDL_RenderFillRect(Renderer, &rect);
}

void GRAPHICS_GEOMETRY_SDL::DrawBoxA(int x1, int y1, int x2, int y2)
{
	DrawWithAlpha([&] { DrawBox(x1, y1, x2, y2); });
}

void GRAPHICS_GEOMETRY_SDL::DrawTriangleFan(VERTEX_XY_SPAN<> xys)
{
	DrawTriangles(TRIANGLE_PRIMITIVE::FAN, xys);
}

void GRAPHICS_GEOMETRY_SDL::DrawLineStrip(VERTEX_XY_SPAN<> xys)
{
	const auto points = HelpFPointsFrom(xys);
	SDL_RenderDrawLinesF(Renderer, points.data(), points.size());
}

void GRAPHICS_GEOMETRY_SDL::DrawTriangles(
	TRIANGLE_PRIMITIVE tp, VERTEX_XY_SPAN<> xys, VERTEX_RGBA_SPAN<> colors
)
{
	if(colors.empty()) {
		const VERTEX_RGBA single = { Col.r, Col.g, Col.b, 0xFF };
		DrawGeometry(tp, xys, std::span(&single, 1));
	} else {
		DrawGeometry(tp, xys, colors);
	}
}

void GRAPHICS_GEOMETRY_SDL::DrawTrianglesA(
	TRIANGLE_PRIMITIVE tp, VERTEX_XY_SPAN<> xys, VERTEX_RGBA_SPAN<> colors
)
{
	DrawWithAlpha([&] {
		if(colors.empty()) {
			const VERTEX_RGBA single = { Col.r, Col.g, Col.b, Col.a };
			DrawGeometry(tp, xys, std::span(&single, 1));
		} else {
			DrawGeometry(tp, xys, colors);
		}
	});
}

void GRAPHICS_GEOMETRY_SDL::DrawGrdLineEx(int x, int y1, RGB c1, int y2, RGB c2)
{
	const auto c1a = c1.WithAlpha(0xFF);
	const auto c2a = c2.WithAlpha(0xFF);
	const VERTEX_XY xys[4] = {
		{ static_cast<VERTEX_COORD>(x + 0), static_cast<VERTEX_COORD>(y1) },
		{ static_cast<VERTEX_COORD>(x + 0), static_cast<VERTEX_COORD>(y2) },
		{ static_cast<VERTEX_COORD>(x + 1), static_cast<VERTEX_COORD>(y1) },
		{ static_cast<VERTEX_COORD>(x + 1), static_cast<VERTEX_COORD>(y2) },
	};
	const VERTEX_RGBA colors[4] = { c1a, c2a, c1a, c2a };
	DrawGeometry(TRIANGLE_PRIMITIVE::STRIP, xys, colors);
}

void GRAPHICS_GEOMETRY_SDL::DrawPoint(WINDOW_POINT) {}
void GRAPHICS_GEOMETRY_SDL::DrawHLine(int, int, int) {}
/// --------

/// Software rendering with pixel access
/// ------------------------------------

bool GrpBackend_PixelAccessStart(void)
{
	if(SoftwareRenderer) {
		return true;
	}
	SoftwareTexture = HelpCreateTexture(
		SoftwareSurface->format->format,
		SDL_TEXTUREACCESS_STREAMING,
		SoftwareSurface->w,
		SoftwareSurface->h
	);
	if(!SoftwareTexture) {
		Log_Fail(LOG_CAT, "Error creating software rendering texture");
		return DestroySoftwareRenderer();
	}
	SoftwareRenderer = SDL_CreateSoftwareRenderer(SoftwareSurface);
	if(!SoftwareRenderer) {
		Log_Fail(LOG_CAT, "Error creating software renderer");
		return DestroySoftwareRenderer();
	}
	SwitchActiveRenderer(SoftwareRenderer);
	return true;
}

bool GrpBackend_PixelAccessEnd(void)
{
	if(!SoftwareRenderer) {
		return true;
	}
	SwitchActiveRenderer(PrimaryRenderer);
	DestroySoftwareRenderer();
	return true;
}

std::tuple<std::byte *, size_t> GrpBackend_PixelAccessLock(void)
{
	// Necessary in SDL 3!
	SDL_RenderFlush(SoftwareRenderer);

	if(SDL_MUSTLOCK(SoftwareSurface)) {
		if(SDL_LockSurface(SoftwareSurface) != 0) {
			Log_Fail(LOG_CAT, "Error locking CPU backbuffer");
			return { nullptr, 0 };
		}
	}
	auto *pixels = static_cast<std::byte *>(SoftwareSurface->pixels);
	return { pixels, SoftwareSurface->pitch };
}

void GrpBackend_PixelAccessUnlock(void)
{
	if(SDL_MUSTLOCK(SoftwareSurface)) {
		SDL_UnlockSurface(SoftwareSurface);
	}
}
/// ------------------------------------
