/*
 *   SDL window creation
 *
 */

#include "platform/window_backend.h"
#include "platform/sdl/log_sdl.h"
#include "platform/snd_backend.h"
#include "platform/input.h"
#include "constants.h"
#include "game/bgm.h"
#include "game/frame.h"
#include "game/graphics.h"
#include <SDL_events.h>
#include <SDL_mouse.h>
#include <SDL_timer.h>
#include <SDL_video.h>
#ifdef WIN32
	#include <SDL_syswm.h>
#endif

constexpr auto LOG_CAT = SDL_LOG_CATEGORY_VIDEO;

static SDL_Window *Window;

SDL_Window *WndBackend_SDLCreate(void)
{
	assert(Window == nullptr);

	// For now, we need to keep resizability for external DirectDraw windowing
	// tools, since we would disallow resizing otherwise. As a side effect,
	// this also allows these tools to freely change the window's aspect ratio
	// as well. Which we can't prevent, since we don't receive window resize
	// messages from SDL in this "resizable fullscreen mode". (Or window move
	// messages, for that matter.)
	constexpr uint32_t flags = (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_RESIZABLE);
	Window = SDL_CreateWindow(GAME_TITLE, 0, 0, GRP_RES.w, GRP_RES.h, flags);
	if(!Window) {
		Log_Fail(LOG_CAT, "Error creating SDL window");
		return nullptr;
	}
	SDL_ShowCursor(false);

	return Window;
}

#ifdef WIN32
HWND WndBackend_Win32Create(void)
{
	auto* window = WndBackend_SDLCreate();

	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version);
	if(!SDL_GetWindowWMInfo(window, &wminfo)) {
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

int WndBackend_Run(void)
{
	bool quit = false;
	bool active = true;
	uint64_t ticks_last = 0;

	while(!quit) {
		// Read input events first to remove them from the queue
		SDL_PumpEvents();
		Key_Read();

		[[gsl::suppress(type.5)]] SDL_Event event;
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
