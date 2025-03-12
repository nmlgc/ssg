/*
 *   SDL window creation
 *
 */

// SDL headers must come first to avoid import→#include bugs on Clang 19.
#ifdef SDL3
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#else
#include <SDL_events.h>
#include <SDL_hints.h>
#include <SDL_mouse.h>
#include <SDL_rect.h>
#include <SDL_render.h>
#include <SDL_timer.h>
#include <SDL_video.h>
#ifdef WIN32_VINTAGE
#include <SDL_syswm.h>
#endif
#endif
#include "platform/sdl/sdl2_wrap.h"

#include "platform/window_backend.h"
#include "platform/sdl/log_sdl.h"
#include "platform/sdl/window_sdl.h"
#include "platform/graphics_backend.h"
#include "platform/snd_backend.h"
#include "platform/input.h"
#include "constants.h"
#include "game/bgm.h"
#include "game/frame.h"
#include "game/graphics.h"

constexpr auto LOG_CAT = SDL_LOG_CATEGORY_VIDEO;

static SDL_Window *Window;
std::optional<std::pair<int16_t, int16_t>> TopleftBeforeFullscreen;

std::pair<int16_t, int16_t> HelpGetWindowPosition(SDL_Window *window)
{
	int left{}, top{};
	SDL_GetWindowPosition(window, &left, &top);
	assert(left > (std::numeric_limits<int16_t>::min)());
	assert(top > (std::numeric_limits<int16_t>::min)());
	assert(left <= (std::numeric_limits<int16_t>::max)());
	assert(top <= (std::numeric_limits<int16_t>::max)());
	return std::make_pair(
		static_cast<int16_t>(left), static_cast<int16_t>(top)
	);
}

SDL_DisplayID HelpGetDisplayForWindow(void)
{
#ifdef SDL3
	if(!Window) {
		return SDL_GetPrimaryDisplay();
	}
	const auto ret = SDL_GetDisplayForWindow(Window);
	if(ret == 0) {
		return SDL_GetPrimaryDisplay();
	}
	return ret;
#else
	if(!Window) {
		return 0;
	}
	return std::max(SDL_GetWindowDisplayIndex(Window), 0);
#endif
}

// Don't do a ZUN.
// (https://github.com/thpatch/thcrap/commit/71c1dcab690f85653cbc9a06c7c55)
SDL_Rect ClampWindowRect(SDL_Rect window_rect)
{
	// SDL_GetDisplayForRect() returns the *closest* display, so we need to
	// manually clamp the window to its bounds.
#ifdef SDL3
	const auto display_i = SDL_GetDisplayForRect(&window_rect);
	if(display_i == 0) {
		return window_rect;
	}
#else
	const auto display_i = SDL_GetRectDisplayIndex(&window_rect);
	if(display_i < 0) {
		return window_rect;
	}
#endif
	SDL_Rect display_rect{};
	if(SDL_GetDisplayUsableBounds(display_i, &display_rect) != 0) {
		return window_rect;
	}

	const auto clamp_start = [](
		int win_start, int win_extent, int disp_start, int disp_extent
	) {
		if(
			SDL_WINDOWPOS_ISCENTERED(win_start) ||
			SDL_WINDOWPOS_ISUNDEFINED(win_start)
		) {
			return win_start;
		}
		const auto win_end = (win_start + win_extent);
		const auto disp_end = (disp_start + disp_extent);
		if(win_end < disp_start) {
			return disp_start;
		} else if(win_end > disp_end) {
			return (disp_end - win_extent);
		}
		return win_start;
	};

	window_rect.x = clamp_start(
		window_rect.x, window_rect.w, display_rect.x, display_rect.w
	);
	window_rect.y = clamp_start(
		window_rect.y, window_rect.h, display_rect.y, display_rect.h
	);
	return window_rect;
}

#ifdef SDL3
std::optional<GRAPHICS_FULLSCREEN_FLAGS> HelpSetFullscreenMode(
	SDL_Window *window, GRAPHICS_FULLSCREEN_FLAGS fs
)
{
	if(fs.fullscreen && fs.exclusive) {
		#pragma warning(suppress : 26494) // type.5
		SDL_DisplayMode mode;

		constexpr auto rate = (1000.0f / FRAME_TIME_TARGET);
		if(!SDL_GetClosestFullscreenDisplayMode(
			HelpGetDisplayForWindow(), GRP_RES.w, GRP_RES.h, rate, false, &mode
		)) {
			Log_Fail(
				LOG_CAT,
				"Could not find a display mode for exclusive fullscreen, falling back on borderless"
			);
			fs.exclusive = false;
			return HelpSetFullscreenMode(window, fs);
		}
		SDL_SetWindowFullscreenMode(window, &mode);
	} else {
		SDL_SetWindowFullscreenMode(window, nullptr);
	}
	if(!SDL_SetWindowFullscreen(window, fs.fullscreen)) {
		Log_Fail(LOG_CAT, "Error changing display mode");
		return std::nullopt;
	}
	return fs;
}
#endif

std::u8string_view HelpDriverName(int8_t id)
{
#ifdef SDL3
	const auto ret = SDL_GetRenderDriver(id);
#else
	SDL_RendererInfo info;
	const auto ret = (
		(SDL_GetRenderDriverInfo(id, &info) == 0) ? info.name : ""
	);
#endif
	return reinterpret_cast<const char8_t *>(ret);
}

int8_t WndBackend_ValidateRenderDriver(const std::u8string_view hint)
{
	for(const auto i : std::views::iota(0, SDL_GetNumRenderDrivers())) {
		if(HelpDriverName(i) == hint) {
			return i;
		}
	}
	const auto *default_driver = (GRP_SDL_DEFAULT_API
		? GRP_SDL_DEFAULT_API
		: HelpDriverName(0).data()
	);
	SDL_LogCritical(
		LOG_CAT,
		"Unsupported renderer \"%s\" specified in " SDL_HINT_RENDER_DRIVER
		" hint, falling back to %s default (%s).",
		std::bit_cast<const char *>(hint.data()),
		(GRP_SDL_DEFAULT_API ? "the" : "SDL's"),
		std::bit_cast<const char *>(default_driver)
	);
#ifdef SDL3
	SDL_UnsetEnvironmentVariable(SDL_GetEnvironment(), SDL_HINT_RENDER_DRIVER);
#else
	SDL_setenv(SDL_HINT_RENDER_DRIVER, "", 1);
#endif
	// If this succeeds, the hint came from SDL, not the environment.
	if(SDL_GetHint(SDL_HINT_RENDER_DRIVER)) {
		SDL_SetHintWithPriority(
			SDL_HINT_RENDER_DRIVER, nullptr, SDL_HINT_OVERRIDE
		);
	}
	return -1;
}

std::u8string_view WndBackend_SDLRendererName(int8_t id)
{
	assert(id < SDL_GetNumRenderDrivers());
	if(id >= 0) {
		return HelpDriverName(id);
	}

	auto *hint = std::bit_cast<const char8_t *>(
		SDL_GetHint(SDL_HINT_RENDER_DRIVER)
	);
	if(!hint) {
		hint = GRP_SDL_DEFAULT_API;
	}
	if(!hint || (hint[0] == '\0')) {
		// SDL tries to initialize drivers in order.
		return HelpDriverName(0);
	}
	id = WndBackend_ValidateRenderDriver(hint);
	if(id < 0) {
		return (GRP_SDL_DEFAULT_API ? GRP_SDL_DEFAULT_API : HelpDriverName(0));
	}
	return HelpDriverName(id);
}

SDL_Window *WndBackend_SDL(void)
{
	return Window;
}

std::optional<GRAPHICS_PARAMS> WndBackend_Create(GRAPHICS_PARAMS params)
{
	assert(Window == nullptr);

	uint32_t flags = 0;

#ifndef WIN32_VINTAGE
	if(GRP_SDL_DEFAULT_API) {
		if((params.api < 0) && !SDL_GetHint(SDL_HINT_RENDER_DRIVER)) {
			SDL_SetHint(
				SDL_HINT_RENDER_DRIVER,
				std::bit_cast<const char *>(GRP_SDL_DEFAULT_API)
			);
		}
	}

	// Set the necessary window flags for certain APIs to avoid
	// SDL_CreateRenderer()'s janky closing and reopening of the window with
	// the correct flags.
	const auto name = WndBackend_SDLRendererName(params.api);

	if(name.starts_with(u8"opengl")) {
		flags |= SDL_WINDOW_OPENGL;
		SDL_GL_ResetAttributes();

		// SDL_GL_ResetAttributes() also resets the essential profile mask and
		// version selection attributes, but chooses the target OpenGL version
		// via a hardcoded #ifdef priority list that prefers regular OpenGL
		// over ES 2 over ES 1. If the user requested any of the ES versions,
		// SDL_CreateRenderer() would still recreate the window because the
		// `SDL_GL_CONTEXT_PROFILE_MASK` got set to regular/non-ES OpenGL.
		// So, we're forced to specify the correct flags ourselves after all.
		const auto [maj, min] = ([name]() -> std::pair<int, int> {
			if(name.starts_with(u8"opengles")) {
				SDL_GL_SetAttribute(
					SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES
				);
				if(name == u8"opengles2") {
					return { 2, OPENGL_TARGET_ES2_MIN };
				}
				return { 1, OPENGL_TARGET_ES1_MIN };
			}
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 0);
			return { OPENGL_TARGET_CORE_MAJ, OPENGL_TARGET_CORE_MIN };
		})();
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, maj);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, min);
	}
#endif

	if((params.left != 0) || (params.top != 0)) {
		TopleftBeforeFullscreen = { params.left, params.top };
	}

	const auto real_pos = [](int16_t pos) -> int {
		return (
			(pos == GRAPHICS_TOPLEFT_UNDEFINED) ? SDL_WINDOWPOS_CENTERED : pos
		);
	};

	const auto res = params.ScaledRes();
	const auto fs = params.FullscreenFlags();
	SDL_Rect rect = {
		.x = real_pos(params.left),
		.y = real_pos(params.top),
		.w = res.w,
		.h = res.h,
	};
	if(!fs.fullscreen) {
		rect = ClampWindowRect(rect);
	}

#ifdef SDL3
	const SDL_PropertiesID props = SDL_CreateProperties();
	SDL_SetStringProperty(
		props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, GAME_TITLE
	);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, rect.x);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, rect.y);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, rect.w);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, rect.h);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER, flags);

	// SDL 3 can only enter exclusive fullscreen after the window has been
	// created, so...
	SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_HIDDEN_BOOLEAN, true);

	Window = SDL_CreateWindowWithProperties(props);
	SDL_DestroyProperties(props);
#else
	flags |= HelpFullscreenFlag(fs);
	Window = SDL_CreateWindow(
		GAME_TITLE, rect.x, rect.y, rect.w, rect.h, flags
	);
#endif
	if(!Window) {
		Log_Fail(LOG_CAT, "Error creating SDL window");
		return std::nullopt;
	}
#ifdef SDL3
	const auto maybe_fs_actual = HelpSetFullscreenMode(Window, fs);
	if(!maybe_fs_actual) {
		SDL_DestroyWindow(Window);
		return std::nullopt;
	}
	const auto fs_actual = maybe_fs_actual.value();
	using F = GRAPHICS_PARAM_FLAGS;
	params.SetFlag(F::FULLSCREEN, fs_actual.fullscreen);
	params.SetFlag(F::FULLSCREEN_EXCLUSIVE, fs_actual.exclusive);
	SDL_ShowWindow(Window);
#endif
	return params;
}

#ifdef WIN32_VINTAGE
HWND WndBackend_Win32(void)
{
	if(!Window) {
		return nullptr;
	}
	HWND ret = nullptr;
#ifdef SDL3
	ret = static_cast<HWND>(SDL_GetPointerProperty(
		SDL_GetWindowProperties(Window),
		SDL_PROP_WINDOW_WIN32_HWND_POINTER,
		nullptr
	));
#else
	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version);
	if(SDL_GetWindowWMInfo(Window, &wminfo)) {
		ret = wminfo.info.win.window;
	}
#endif
	if(!ret) {
		Log_Fail(LOG_CAT, "Error retrieving window handle");
	}
	return ret;
}
#endif

void WndBackend_Cleanup(void)
{
	if(Window) {
		SDL_DestroyWindow(Window);
		Window = nullptr;
	}
}

std::optional<std::pair<int16_t, int16_t>> WndBackend_Topleft(void)
{
	// A fullscreen window is always positioned at (0, 0), and we don't want to
	// override any previous windowed position.
	if(!Window || (SDL_GetWindowFlags(Window) & SDL_WINDOW_FULLSCREEN)) {
		return TopleftBeforeFullscreen;
	}
	return HelpGetWindowPosition(Window);
}

int WndBackend_Run(void)
{
	bool quit = false;
	bool active = true;
	uint64_t ticks_last = 0;

	while(!quit) {
		// Read input events first to remove them from the queue
		SDL_PumpEvents();
		Key_Read();

		SDL_Event event;
		while(SDL_PeepEvents(
			&event, 1, SDL_GETEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST
		) == 1) {
			switch(event.type) {
			case SDL_EVENT_QUIT:
				return 0;

#ifdef SDL2
			case SDL_WINDOWEVENT: switch(event.window.event) {
#endif
			case SDL_EVENT_WINDOW_FOCUS_LOST:
				BGM_Pause();
				SndBackend_PauseAll();
				active = false;
				break;

			case SDL_EVENT_WINDOW_FOCUS_GAINED:
				BGM_Resume();
				SndBackend_ResumeAll();
				active = true;
				break;
#ifdef SDL2
			default: break; }
			break;
#endif

			default:
				break;
			}
		}

		if(active) {
			const auto ticks_start = SDL_GetTicks();
			if(
				(Grp_FPSDivisor == 0) ||
				((ticks_start - ticks_last) >= FRAME_TIME_TARGET)
			) {
				quit = !GameFrame();
				if(Grp_FPSDivisor != 0) {
					// Since SDL_Delay() works at not-even-exact millisecond
					// granularity, we subtract 1 and spin for the last
					// millisecond to ensure that we hit the exact frame
					// boundary.
					const auto ticks_frame = (SDL_GetTicks() - ticks_start);
					if(ticks_frame < (FRAME_TIME_TARGET - 1)) {
						SDL_Delay((FRAME_TIME_TARGET - 1) - ticks_frame);
					}
					ticks_last = ticks_start;
				}
			}
		} else {
			SDL_WaitEvent(nullptr);
		}
	}
	return 0;
}
