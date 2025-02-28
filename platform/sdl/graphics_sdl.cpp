/*
 *   Graphics via SDL_Renderer
 *
 */

// SDL headers must come first to avoid import→#include bugs on Clang 19.
#ifdef SDL3
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_render.h>

using SDL_COLOR = SDL_FColor;
#else
#include <SDL_mouse.h>
#include <SDL_render.h>

using SDL_COLOR = SDL_Color;
#endif
#include "platform/sdl/sdl2_wrap.h"

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

std::optional<PIXELFORMAT> PreferredPixelFormat;
SDL_ScaleMode TextureScaleMode = SDL_SCALEMODE_NEAREST;

// Primary renderer
// ----------------
// Probably hardware-accelerated, but not necessarily.

SDL_Renderer *PrimaryRenderer = nullptr;
#ifdef SDL2
SDL_RendererInfo PrimaryInfo;
#endif

// Same lifetime as either [PrimaryRenderer] (SDL 3) or [PrimaryInfo] (SDL 2).
std::span<const SDL_PixelFormat> PrimaryFormats;

// Rendering target in framebuffer scale mode. Not to be confused with
// [SoftwareTexture], which is a streaming texture, not a render target.
SDL_Texture *PrimaryTexture = nullptr;

#ifdef SDL2
// Buffer for screenshots at non-native resolutions when using geometry
// scaling. Allocated on demand… in SDL 2, at least, SDL 3 insists on
// allocating a new surface on every call to SDL_RenderReadPixels().
SDL_Surface *GeometryScaledBackbuffer = nullptr;
#endif
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

SDL_PixelFormat HelpFormatFrom(const SDL_Surface *surface)
{
#ifdef SDL3
	return surface->format;
#else
	return surface->format->format;
#endif
}

std::optional<PIXELFORMAT> HelpPixelFormatFrom(SDL_PixelFormat format)
{
	if((SDL_BITSPERPIXEL(format) == 32) && SDL_ISPIXELFORMAT_PACKED(format)) {
		return std::make_optional<PIXELFORMAT>(uint32_t{});
	}
	return std::nullopt;
}

template <typename Rect> Rect HelpRectTo(const PIXEL_LTWH& o) noexcept
{
	return Rect{
		.x = static_cast<decltype(Rect::x)>(o.left),
		.y = static_cast<decltype(Rect::y)>(o.top),
		.w = static_cast<decltype(Rect::w)>(o.w),
		.h = static_cast<decltype(Rect::h)>(o.h),
	};
}

template <typename Rect> Rect HelpRectTo(const PIXEL_LTRB& o)
{
	return Rect{
		.x = static_cast<decltype(Rect::x)>(o.left),
		.y = static_cast<decltype(Rect::y)>(o.top),
		.w = static_cast<decltype(Rect::w)>(o.right - o.left),
		.h = static_cast<decltype(Rect::h)>(o.bottom - o.top),
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

std::span<const SDL_COLOR> HelpColorsFrom(VERTEX_RGBA_SPAN<> sp)
{
	using GT = decltype(sp)::value_type;
	static_assert(sizeof(SDL_COLOR) == sizeof(GT));
	static_assert(std::is_same_v<decltype(SDL_COLOR::r), decltype(GT::r)>);
	static_assert(std::is_same_v<decltype(SDL_COLOR::g), decltype(GT::g)>);
	static_assert(std::is_same_v<decltype(SDL_COLOR::b), decltype(GT::b)>);
	static_assert(std::is_same_v<decltype(SDL_COLOR::a), decltype(GT::a)>);
	return { std::bit_cast<const SDL_COLOR *>(sp.data()), sp.size() };
}

SDL_Texture *TexturePostInit(SDL_Texture& tex)
{
#ifdef SDL2
	SDL_SetTextureUserData(&tex, Renderer);
#endif
	SDL_SetTextureScaleMode(&tex, TextureScaleMode);
	return &tex;
}

SDL_Texture *HelpCreateTexture(
	SDL_PixelFormat format, SDL_TextureAccess access, int w, int h
)
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
#ifdef SDL3
public:
	SDL2_RENDER_TARGET_QUIRK_WORKAROUND(SDL_Renderer& renderer) {}
	~SDL2_RENDER_TARGET_QUIRK_WORKAROUND() {}
#else
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
#endif
};

template <typename T> [[nodiscard]] T *SafeDestroy(void Destroy(T *), T *v)
{
	if(v) {
		Destroy(v);
		v = nullptr;
	}
	return v;
}

bool SetRenderTargetFor(const SDL_Renderer *renderer)
{
	if(renderer == SoftwareRenderer) {
		return !HelpFailed(SDL_SetRenderTarget(PrimaryRenderer, nullptr));
	} else if((renderer == PrimaryRenderer) && PrimaryTexture) {
		return !HelpFailed(
			SDL_SetRenderTarget(PrimaryRenderer, PrimaryTexture)
		);
	}
	return true;
}

void SwitchActiveRenderer(SDL_Renderer *new_renderer)
{
	for(auto& tex : Textures) {
		if(!tex) {
			continue;
		}
		const auto *renderer = (
#ifdef SDL3
			SDL_GetRendererFromTexture(tex)
#else
			SDL_GetTextureUserData(tex)
#endif
		);
		if(tex && (renderer == Renderer)) {
			tex = SafeDestroy(SDL_DestroyTexture, tex);
		}
	}
	SetRenderTargetFor(new_renderer);
	Renderer = new_renderer;
}

std::optional<GRAPHICS_FULLSCREEN_FLAGS> HelpSwitchFullscreen(
	const GRAPHICS_FULLSCREEN_FLAGS& fs_prev,
	const GRAPHICS_FULLSCREEN_FLAGS& fs_new
)
{
	auto *window = WndBackend_SDL();

	if(!fs_prev.fullscreen && fs_new.fullscreen) {
		TopleftBeforeFullscreen = HelpGetWindowPosition(window);
	}

#ifdef SDL3
	const auto fs_actual = HelpSetFullscreenMode(window, fs_new);
	if(!fs_actual) {
		return std::nullopt;
	}
#else
	if(SDL_SetWindowFullscreen(window, HelpFullscreenFlag(fs_new)) != 0) {
		Log_Fail(LOG_CAT, "Error changing display mode");
		return std::nullopt;
	}
	const auto& fs_actual = fs_new;
#endif

	// If we come out of fullscreen mode, recenter the window.
	if(fs_prev.fullscreen && !fs_new.fullscreen) {
		const auto display_i = HelpGetDisplayForWindow();
		const auto center = SDL_WINDOWPOS_CENTERED_DISPLAY(display_i);
		SDL_SetWindowPosition(window, center, center);
	}
	return fs_actual;
}
// -------

// Pretty API version strings
// --------------------------

constexpr std::pair<std::u8string_view, std::u8string_view> API_NICE[] = {
	{ u8"direct3d", u8"Direct3D 9" },
	{ u8"direct3d11", u8"Direct3D 11" },
	{ u8"direct3d12", u8"Direct3D 12" },
	{ u8"software", u8"Software" },
	{ u8"vulkan", u8"Vulkan" },
};

namespace APIVersions {
constexpr const char GPU_FMT[] = "GPU (%s)";
constexpr const char OPENGL_FMT[] = "%s %d.%d";
constexpr size_t PRETTY_SIZE_MAX = 9;
constexpr size_t API_SIZE_MAX = std::ranges::max_element(
	API_NICE, [](const auto& a, const auto& b) {
		return (a.second.size() < b.second.size());
	}
)->second.size();
constexpr size_t FMT_SIZE = std::max(
	(sizeof(GPU_FMT) + API_SIZE_MAX),
	(PRETTY_SIZE_MAX + 1 + STRING_NUM_CAP<int> + 1 + STRING_NUM_CAP<int>)
);

struct VERSION {
	std::u8string_view name_sdl;
	const char *name_pretty;
	void (*update)(VERSION& self);
	char8_t buf[FMT_SIZE + 1];
};

#ifdef SDL3
void UpdateGPU(VERSION& self)
{
	const auto props = SDL_GetRendererProperties(PrimaryRenderer);
	auto *gpu_device = static_cast<SDL_GPUDevice *>(SDL_GetPointerProperty(
		props, SDL_PROP_RENDERER_GPU_DEVICE_POINTER, nullptr
	));

	std::u8string_view device_name = reinterpret_cast<const char8_t *>(
		SDL_GetGPUDeviceDriver(gpu_device)
	);
	for(const auto& nice : API_NICE) {
		if(nice.first == device_name) {
			device_name = nice.second;
			break;
		}
	}
	auto *buf = reinterpret_cast<char *>(self.buf);
	const auto *via_name = (device_name.empty()
		? "?"
		: reinterpret_cast<const char *>(device_name.data())
	);
	sprintf(&buf[0], &GPU_FMT[0], via_name);
}
#endif

void UpdateOpenGL(VERSION& self)
{
	int major = 0;
	int minor = 0;
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);

	auto* buf = reinterpret_cast<char *>(self.buf);
	sprintf(&buf[0], &OPENGL_FMT[0], self.name_pretty, major, minor);
}

#define TARGET_(pretty, maj, min) pretty " ~" #maj "." #min
#define TARGET(pretty, maj, min) TARGET_(pretty, maj, min)

constinit VERSION Versions[] = {
#ifdef SDL3
	{ u8"gpu", nullptr, UpdateGPU, u8"GPU" },
#endif
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
	const auto display_i = HelpGetDisplayForWindow();
	if(fullscreen) {
#ifdef SDL3
		const auto *display_mode = SDL_GetDesktopDisplayMode(display_i);
		if(!display_mode) {
#else
		SDL_DisplayMode display_mode_local{};
		const auto *display_mode = &display_mode_local;
		if(SDL_GetDesktopDisplayMode(display_i, &display_mode_local) != 0) {
#endif
			Log_Fail(LOG_CAT, "Error retrieving display size");
			return GRP_RES;
		}
		return { .w = display_mode->w, .h = display_mode->h };
	}

	if(HelpFailed(SDL_GetDisplayUsableBounds(display_i, &rect))) {
		Log_Fail(LOG_CAT, "Error retrieving display size");
		return GRP_RES;
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
#ifdef SDL3
			SDL_SetRenderLogicalPresentation(
				PrimaryRenderer,
				GRP_RES.w,
				GRP_RES.h,
				SDL_LOGICAL_PRESENTATION_STRETCH
			);
#else
			SDL_RenderSetLogicalSize(PrimaryRenderer, GRP_RES.w, GRP_RES.h);
			// [wa] must drop after SDL_RenderSetLogicalSize()!
#endif
		} else {
#ifdef SDL3
			SDL_SetRenderLogicalPresentation(
				PrimaryRenderer,
				GRP_RES.w,
				GRP_RES.h,
				SDL_LOGICAL_PRESENTATION_STRETCH
			);
#else
			SDL_RenderSetLogicalSize(PrimaryRenderer, GRP_RES.w, GRP_RES.h);
#endif
		}
		return true;
	};

	// Update texture filters
	// ----------------------
	if((scaled_res.w % GRP_RES.w) || (scaled_res.h % GRP_RES.h)) {
		TextureScaleMode = SDL_SCALEMODE_LINEAR;
	} else {
		TextureScaleMode = SDL_SCALEMODE_NEAREST;
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

#ifdef SDL2
	GeometryScaledBackbuffer = SafeDestroy(
		SDL_DestroySurface, GeometryScaledBackbuffer
	);
#endif
	if(geometry || (scaled_res == GRP_RES)) {
		set_geometry();
		return geometry; // Don't unset the user's choice on 1× scaling!
	}

	assert(!geometry);
#ifdef SDL2
	if(!SDL_RenderTargetSupported(PrimaryRenderer)) {
		Log_Fail(LOG_CAT, "Render API does not support render targets");
		return set_geometry();
	}
#endif

	// Prepare the primary renderer for blitting the primary texture:
	// • Ensure the correct logical size
	// • Move the renderer's clipping rectangle onto whatever new renderer we
	//   target (won't be needed in SDL 3)
	// • Stop clipping on the primary renderer!!! Its clipping region is going
	//   to apply to the [PrimaryTexture] blit going forward!!!
	const auto wa = SDL2_RENDER_TARGET_QUIRK_WORKAROUND{ *Renderer };
	SDL_SetRenderTarget(PrimaryRenderer, nullptr);
	SDL_SetRenderClipRect(PrimaryRenderer, nullptr);
#ifdef SDL3
	SDL_SetRenderLogicalPresentation(
		PrimaryRenderer,
		scaled_res.w,
		scaled_res.h,
		SDL_LOGICAL_PRESENTATION_DISABLED
	);
#else
	SDL_RenderSetLogicalSize(PrimaryRenderer, scaled_res.w, scaled_res.h);
#endif

	if(!PrimaryTexture) {
		assert(SoftwareSurface);
		const auto format = HelpFormatFrom(SoftwareSurface);
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
	if(!SetRenderTargetFor(Renderer)) {
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

#ifdef SDL3
	const auto display_i = SDL_GetDisplayForWindow(window);
	if(display_i == 0) {
		return topleft_prev;
	}
#else
	const auto display_i = SDL_GetWindowDisplayIndex(window);
	if(display_i < 0) {
		return topleft_prev;
	}
#endif
	SDL_Rect display_r{};
	if(HelpFailed(SDL_GetDisplayUsableBounds(display_i, &display_r))) {
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
#ifdef SDL3
		constexpr auto MODES = [] {
			ENUMARRAY<SDL_RendererLogicalPresentation, FIT> ret;
			ret[FIT::INTEGER] = SDL_LOGICAL_PRESENTATION_INTEGER_SCALE;
			ret[FIT::ASPECT] = SDL_LOGICAL_PRESENTATION_LETTERBOX;
			ret[FIT::STRETCH] = SDL_LOGICAL_PRESENTATION_STRETCH;
			return ret;
		}();
		SDL_SetRenderLogicalPresentation(
			PrimaryRenderer, GRP_RES.w, GRP_RES.h, MODES[fs.fit]
		);
#else
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
#endif
	}
	SDL_SetRenderTarget(PrimaryRenderer, target);
}

std::optional<GRAPHICS_INIT_RESULT> PrimaryInitFull(GRAPHICS_PARAMS params)
{
	const auto maybe_params = WndBackend_Create(params);
	if(!maybe_params) {
		return std::nullopt;
	}
	params = maybe_params.value();

#ifdef SDL3
	const auto *driver = SDL_GetRenderDriver(params.api);
	auto *renderer = SDL_CreateRenderer(WndBackend_SDL(), driver);
#else
	auto *renderer = SDL_CreateRenderer(
		WndBackend_SDL(), params.api, SDL_RENDERER_ACCELERATED
	);
#endif
	if(!renderer) {
		const auto api_name = GrpBackend_APIName(params.api);
		const auto* api = std::bit_cast<const char *>(api_name.data());
		SDL_LogCritical(
			LOG_CAT, "Error creating %s renderer: %s", api, SDL_GetError()
		);
		WndBackend_Cleanup();
		return std::nullopt;
	}

#ifdef SDL3
	const auto props = SDL_GetRendererProperties(renderer);
	const auto *formats_start = static_cast<const SDL_PixelFormat *>(
		SDL_GetPointerProperty(
			props, SDL_PROP_RENDERER_TEXTURE_FORMATS_POINTER, nullptr
		)
	);
	auto *formats_end = formats_start;
	while(*formats_end != SDL_PIXELFORMAT_UNKNOWN) {
		formats_end++;
	}
	PrimaryFormats = {
		formats_start,
		static_cast<size_t>(formats_end - formats_start),
	};
#else
	SDL_GetRendererInfo(renderer, &PrimaryInfo);
	PrimaryFormats = std::span(
		&PrimaryInfo.texture_formats[0], PrimaryInfo.num_texture_formats
	);
#endif

	// Determine the preferred texture format. We don't overwrite
	// [params.bitdepth] here to allow frictionless switching between the old
	// DirectDraw/Direct3D backend and this one.
	// SDL_GetWindowPixelFormat() is *not* a shortcut we could use for software
	// rendering mode. On my system, it always returns the 24-bit RGB888, which
	// we don't support.
	SDL_PixelFormat sdl_format = SDL_PIXELFORMAT_UNKNOWN;
	for(const auto format : PrimaryFormats) {
		// Both screenshots and the Pango/Cairo text backend currently expect
		// ARGB order.
		if(SDL_PIXELORDER(format) == SDL_PACKEDORDER_ABGR) {
			continue;
		}

		PreferredPixelFormat = HelpPixelFormatFrom(format);
		if(PreferredPixelFormat) {
			sdl_format = format;
			break;
		}
	}
	if(!PreferredPixelFormat || (sdl_format == SDL_PIXELFORMAT_UNKNOWN)) {
		const auto api_name = GrpBackend_APIName(params.api);
		SDL_LogCritical(
			LOG_CAT,
			"The \"%s\" renderer does not support any of the game's supported software rendering pixel formats.",
			std::bit_cast<const char *>(api_name.data())
		);
		return std::nullopt;
	}

	PrimaryRenderer = renderer;
	SwitchActiveRenderer(PrimaryRenderer);
	APIVersions::Update(params.api);

	// Ensure that the software surface uses the preferred format
	if(!SoftwareSurface || (HelpFormatFrom(SoftwareSurface) != sdl_format)) {
		SoftwareSurface = SafeDestroy(SDL_DestroySurface, SoftwareSurface);
		SoftwareSurface = SDL_CreateSurface(GRP_RES.w, GRP_RES.h, sdl_format);
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
	if(fs_new.fullscreen && fs_new.exclusive) {
#ifdef SDL3
		SDL_HideCursor();
#else
		SDL_ShowCursor(false);
#endif
	} else {
#ifdef SDL3
		SDL_ShowCursor();
#else
		SDL_ShowCursor(true);
#endif
	}

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

#ifdef SDL2
		// If the window started out as borderless and should later switch to
		// exclusive, SDL 2 simply doesn't do the mode change:
		//
		// 	https://github.com/libsdl-org/SDL/issues/11047
		if(fs_prev.fullscreen && !fs_prev.exclusive) {
			return reinit_full(params);
		}
#endif
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
		const auto maybe_fs_actual = HelpSwitchFullscreen(fs_prev, fs_new);
		if(maybe_fs_actual) {
			const auto& fs_actual = maybe_fs_actual.value();

			// If we clipped on the raw renderer, the clipping rectangle won't
			// match the current resolution anymore.
			SDL_SetRenderClipRect(PrimaryRenderer, nullptr);

			ret.live.SetFlag(F::FULLSCREEN, fs_actual.fullscreen);
			ret.live.SetFlag(F::FULLSCREEN_EXCLUSIVE, fs_actual.exclusive);
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

#ifdef SDL3
		// Necessary for the X11 backend.
		SDL_SyncWindow(window);
#endif

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
	SoftwareSurface = SafeDestroy(SDL_DestroySurface, SoftwareSurface);
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
	const auto sdl_rect = HelpRectTo<SDL_Rect>(rect);
	SDL_SetRenderClipRect(Renderer, &sdl_rect);
}

PIXELFORMAT GrpBackend_PixelFormat(void)
{
	if(!PreferredPixelFormat) {
		assert(!"The pixel format should always be valid here");
		std::unreachable();
	}
	return PreferredPixelFormat.value();
}

void GrpBackend_PaletteGet(PALETTE& pal) {}
bool GrpBackend_PaletteSet(const PALETTE& pal) { return false; }

void SaveSurfaceAsScreenshot(
	std::unique_ptr<FILE_STREAM_WRITE> stream, SDL_Surface *src
)
{
	assert(stream);
	assert(src);
	assert(src->pitch >= 0);
#ifdef SDL3
	assert(SDL_GetSurfacePalette(src) == nullptr);
#else
	assert(src->format->palette == nullptr);
#endif

	// Negate the height so that we neither have to flip the pixels nor write
	// RWops for `FILE_STREAM_WRITE` in order to use SDL_SaveBMP_RW().
	const PIXEL_SIZE size = { .w = src->w, .h = -src->h };

	if(SDL_MUSTLOCK(src)) {
		SDL_LockSurface(src);
	}
	const auto pixels = std::span(
		static_cast<std::byte *>(src->pixels), (src->h * src->pitch)
	);
	const auto bpp = SDL_BITSPERPIXEL(HelpFormatFrom(src));
	BMPSave(stream.get(), size, 1, bpp, {}, pixels);
	if(SDL_MUSTLOCK(src)) {
		SDL_UnlockSurface(src);
	}
	stream.reset();
}

void MaybeTakeScreenshot(std::unique_ptr<FILE_STREAM_WRITE> stream)
{
	if(!stream) {
		return;
	}

	if(SoftwareRenderer) {
		// Software rendering is the ideal case for screenshots, because we
		// already have a system-memory surface we can save.
		SaveSurfaceAsScreenshot(std::move(stream), SoftwareSurface);
		return;
	}

#ifdef SDL3
	SDL_Surface *src = SDL_RenderReadPixels(PrimaryRenderer, nullptr);
	if(!src) {
		Log_Fail(LOG_CAT, "Error taking screenshot");
		return;
	}
	defer(SDL_DestroySurface(src));
#else
	SDL_Surface *src = ([] {
		if(PrimaryTexture) {
			// Thankfully we can SDL_RenderReadPixels() from a render target to
			// get a screenshot at the native resolution. [SoftwareSurface]
			// uses the same size, so let's use it as temporary storage.
			return SoftwareSurface;
		}

		// If we aren't scaling, we are lucky and can just do the same we do in
		// the hardware texture case.
		SDL_Rect unscaled_viewport{};
		float scale_x{}, scale_y{};
		SDL_RenderGetViewport(PrimaryRenderer, &unscaled_viewport);
		SDL_RenderGetScale(PrimaryRenderer, &scale_x, &scale_y);
		const PIXEL_COORD scaled_w = (unscaled_viewport.w * scale_x);
		const PIXEL_COORD scaled_h = (unscaled_viewport.h * scale_y);
		if(
			(scaled_w == SoftwareSurface->w) && (scaled_h == SoftwareSurface->h)
		) {
			return SoftwareSurface;
		}

		// If we are, we must take a screenshot at the scaled resolution.
		if(!GeometryScaledBackbuffer) {
			const auto format = SoftwareSurface->format->format;
			GeometryScaledBackbuffer = SDL_CreateRGBSurfaceWithFormat(
				0, scaled_w, scaled_h, 0, format
			);
		}
		return GeometryScaledBackbuffer;
	})();
	if(!src) {
		Log_Fail(LOG_CAT, "Failed to create temporary storage for screenshot");
		return;
	}

	if(SDL_MUSTLOCK(src)) {
		SDL_LockSurface(src);
	}
	if(SDL_RenderReadPixels(
		PrimaryRenderer, nullptr, src->format->format, src->pixels, src->pitch
	) != 0) {
		Log_Fail(LOG_CAT, "Error taking screenshot");
		return;
	}
	if(SDL_MUSTLOCK(src)) {
		SDL_UnlockSurface(src);
	}
#endif
	SaveSurfaceAsScreenshot(std::move(stream), src);
}

void GrpBackend_Flip(std::unique_ptr<FILE_STREAM_WRITE> screenshot_stream)
{
	MaybeTakeScreenshot(std::move(screenshot_stream));
	if(SoftwareRenderer) {
		SDL_FlushRenderer(SoftwareRenderer);
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
		SDL_RenderTexture(PrimaryRenderer, SoftwareTexture, nullptr, nullptr);
		SDL_RenderPresent(PrimaryRenderer);
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

		SDL_RenderTexture(PrimaryRenderer, PrimaryTexture, nullptr, nullptr);

		// SDL_RenderPresent() is not allowed to be called when rendering to a
		// texture, and fails as of SDL 3.2.8:
		//
		// 	https://github.com/libsdl-org/SDL/issues/12432
		SDL_RenderPresent(PrimaryRenderer);
		SDL_SetRenderTarget(PrimaryRenderer, PrimaryTexture);
	} else {
		SDL_RenderPresent(PrimaryRenderer);
	}
}
/// -------

/// Surfaces
/// --------

bool CreateTextureWithFormat(
	SURFACE_ID sid, SDL_PixelFormat fmt, const PIXEL_SIZE& size
)
{
	auto& tex = Textures[sid];
	tex = SafeDestroy(SDL_DestroyTexture, tex);

	tex = HelpCreateTexture(fmt, SDL_TEXTUREACCESS_STREAMING, size.w, size.h);
	if(!tex) {
		Log_Fail(LOG_CAT, "Error creating blank texture");
		return false;
	}
	if(HelpFailed(SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND))) {
		Log_Fail(LOG_CAT, "Error enabling alpha blending for texture");
		return false;
	}
	return true;
}

bool GrpSurface_CreateUninitialized(SURFACE_ID sid, const PIXEL_SIZE& size)
{
	return CreateTextureWithFormat(sid, HelpFormatFrom(SoftwareSurface), size);
}

bool GrpSurface_Load(SURFACE_ID sid, BMP_OWNED&& bmp)
{
	auto& tex = Textures[sid];
	tex = SafeDestroy(SDL_DestroyTexture, tex);

	auto *rwops = SDL_IOFromMem(bmp.buffer.get(), bmp.buffer.size());
	auto *surf = SDL_LoadBMP_IO(rwops, 1);
	defer(SDL_DestroySurface(surf));
	std::ignore = std::move(bmp);

	if(HelpFormatFrom(surf) == SDL_PIXELFORMAT_INDEX8) {
		// The transparent pixel is in the top-left corner.
		if(SDL_MUSTLOCK(surf)) {
			SDL_LockSurface(surf);
		}
		const auto key = static_cast<uint8_t *>(surf->pixels)[0];
		if(SDL_MUSTLOCK(surf)) {
			SDL_UnlockSurface(surf);
		}
		SDL_SetSurfaceColorKey(surf, true, key);
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
	const auto rect = HelpRectTo<SDL_Rect>(*subrect);
	return (SDL_UpdateTexture(tex, &rect, buf, pitch) == 0);
}


PIXEL_SIZE GrpSurface_Size(SURFACE_ID sid)
{
	auto *tex = Textures[sid];
	if(!tex) {
		return { 0, 0 };
	}
#ifdef SDL3
	float w = 0;
	float h = 0;
	if(!SDL_GetTextureSize(tex, &w, &h)) {
		return { 0, 0 };
	}
	return {
		.w = static_cast<PIXEL_COORD>(w),
		.h = static_cast<PIXEL_COORD>(h),
	};
#else
	PIXEL_SIZE ret = { 0, 0 };
	SDL_QueryTexture(tex, nullptr, nullptr, &ret.w, &ret.h);
	return ret;
#endif
}

bool GrpSurface_Blit(
	WINDOW_POINT topleft, SURFACE_ID sid, const PIXEL_LTRB& src
)
{
	const auto tex = Textures[sid];
#ifdef SDL3
	const auto rect_src = HelpRectTo<SDL_FRect>(src);
#else
	const auto rect_src = HelpRectTo<SDL_Rect>(src);
#endif
	const SDL_FRect rect_dst = {
		.x = static_cast<float>(topleft.x),
		.y = static_cast<float>(topleft.y),
		.w = static_cast<float>(rect_src.w),
		.h = static_cast<float>(rect_src.h),
	};
#ifdef SDL3
	return SDL_RenderTexture(Renderer, tex, &rect_src, &rect_dst);
#else
	return (SDL_RenderCopyF(Renderer, tex, &rect_src, &rect_dst) == 0);
#endif
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
static constexpr auto GDITEXT_SDL_FORMAT = SDL_PIXELFORMAT_ARGB8888;

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

	if(!std::ranges::contains(PrimaryFormats, GDITEXT_SDL_FORMAT)) {
		SDL_LogCritical(
			LOG_CAT,
			"Renderer \"%s\" does not support the BGRA8888 pixel format required for rendering text via GDI.",
#ifdef SDL3
			SDL_GetRendererName(PrimaryRenderer)
#else
			PrimaryInfo.name
#endif
		);
		return false;
	};

#ifdef SDL3
	const auto *format_struct = SDL_GetPixelFormatDetails(GDITEXT_SDL_FORMAT);
#else
	auto *format_struct = SDL_AllocFormat(GDITEXT_SDL_FORMAT);
#endif
	if(!format_struct) {
		Log_Fail(
			LOG_CAT, "Error retrieving format structure for GDI text surface"
		);
		return false;
	}
#ifdef SDL2
	defer(SDL_FreeFormat(format_struct));
#endif

	// GDI always sets the "alpha channel" to 0. Removing it from the color key
	// as well saves an OR operation in the alpha fixing loop.
	GDIText_AlphaMask = format_struct->Amask;
	GDIText_ColorKey = (
#ifdef SDL3
		SDL_MapRGB(format_struct, nullptr, colorkey.r, colorkey.g, colorkey.b) &
#else
		SDL_MapRGB(format_struct, colorkey.r, colorkey.g, colorkey.b) &
#endif
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
	SDL_GetRenderScale(Renderer, &offset_x, &offset_y);
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
		((sdl_colors.size() == 1) ? 0 : sizeof(SDL_COLOR)),
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
	SDL_RenderLine(Renderer, x1, y1, x2, y2);
}

void GRAPHICS_GEOMETRY_SDL::DrawBox(int x1, int y1, int x2, int y2)
{
	const SDL_FRect rect = {
		.x = static_cast<float>(x1),
		.y = static_cast<float>(y1),
		.w = static_cast<float>(x2 - x1),
		.h = static_cast<float>(y2 - y1),
	};
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
	SDL_RenderLines(Renderer, points.data(), points.size());
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
		HelpFormatFrom(SoftwareSurface),
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
	SDL_FlushRenderer(SoftwareRenderer);

	if(SDL_MUSTLOCK(SoftwareSurface)) {
		if(HelpFailed(SDL_LockSurface(SoftwareSurface))) {
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
