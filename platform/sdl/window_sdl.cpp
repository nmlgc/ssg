/*
 *   SDL window creation
 *
 */

// SDL headers must come first to avoid import→#include bugs on Clang 19.
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
	if(!Window) {
		return 0;
	}
	return std::max(SDL_GetWindowDisplayIndex(Window), 0);
}

// Don't do a ZUN.
// (https://github.com/thpatch/thcrap/commit/71c1dcab690f85653cbc9a06c7c55)
SDL_Rect ClampWindowRect(SDL_Rect window_rect)
{
	// SDL_GetRectDisplayIndex() returns the *closest* display, so we need to
	// manually clamp the window to its bounds.
	const int display_i = SDL_GetRectDisplayIndex(&window_rect);
	if(display_i < 0) {
		return window_rect;
	}
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

std::u8string_view WndBackend_SDLRendererName(int8_t id)
{
	if(id < 0) {
		const auto ret = SDL_GetHint(SDL_HINT_RENDER_DRIVER);
		if(!ret) {
			return {};
		};
		return reinterpret_cast<const char8_t *>(ret);
	}
	SDL_RendererInfo info;
	if(SDL_GetRenderDriverInfo(id, &info) != 0) {
		return {};
	}
	return reinterpret_cast<const char8_t *>(info.name);
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
	flags |= HelpFullscreenFlag(fs);
	Window = SDL_CreateWindow(
		GAME_TITLE, rect.x, rect.y, rect.w, rect.h, flags
	);
	if(!Window) {
		Log_Fail(LOG_CAT, "Error creating SDL window");
		return std::nullopt;
	}

	return params;
}

#ifdef WIN32_VINTAGE
HWND WndBackend_Win32(void)
{
	if(!Window) {
		return nullptr;
	}
	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version);
	if(!SDL_GetWindowWMInfo(Window, &wminfo)) {
		Log_Fail(LOG_CAT, "Error retrieving window handle");
		return nullptr;
	}
	return wminfo.info.win.window;
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
			&event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT
		) == 1) {
			switch(event.type) {
			case SDL_QUIT:
				return 0;

			case SDL_WINDOWEVENT: {
				switch(event.window.event) {
				case SDL_WINDOWEVENT_FOCUS_LOST:
					BGM_Pause();
					SndBackend_PauseAll();
					active = false;
					break;
				case SDL_WINDOWEVENT_FOCUS_GAINED:
					BGM_Resume();
					SndBackend_ResumeAll();
					active = true;
					break;
				default:
					break;
				}
				break;
			}

			default:
				break;
			}
		}

		if(active) {
			const auto ticks_start = SDL_GetTicks64();
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
					const auto ticks_frame = (SDL_GetTicks64() - ticks_start);
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
