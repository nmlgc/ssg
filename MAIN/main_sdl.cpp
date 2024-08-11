/*
 *   SDL entry point
 *
 */

// Enable visual styles for nice-looking SDL message boxes. Taken from the
// comment in `SDL_windowsmessagebox.c`, which was in turn taken from
//
// 	https://learn.microsoft.com/en-us/windows/win32/controls/cookbook-overview
#pragma comment(linker, \
	"\"/manifestdependency:type='win32'" \
	" name='Microsoft.Windows.Common-Controls'" \
	" version='6.0.0.0'" \
	" processorArchitecture='*'" \
	" publicKeyToken='6595b64144ccf1df'" \
	" language='*'\"" \
)

#include <SDL.h>
#include <SDL_syswm.h>
#include "constants.h"
#include "GIAN07/ENTRY.H"
#include "GIAN07/GAMEMAIN.H"
#include "platform/input.h"
#include "platform/snd_backend.h"
#include "platform/sdl/log_sdl.h"
#include "game/bgm.h"
#include "game/defer.h"
#include "game/frame.h"
#include "game/graphics.h"
#include "strings/title.h"

// Still required for:
// • DirectDraw
HWND hWndMain;

int Run()
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

#define UTF8_(S) u8 ## S
#define UTF8(S) UTF8_(S)

int main(int argc, char** args)
{
	Log_Init(UTF8(GAME_TITLE));

	if(SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_TIMER | SDL_INIT_VIDEO) < 0) {
		Log_Fail(SDL_LOG_CATEGORY_VIDEO, "Error initializing SDL");
		return 1;
	}
	defer(SDL_Quit());

	// For now, we need to keep resizability for external DirectDraw windowing
	// tools, since we would disallow resizing otherwise. As a side effect,
	// this also allows these tools to freely change the window's aspect ratio
	// as well. Which we can't prevent, since we don't receive window resize
	// messages from SDL in this "resizable fullscreen mode". (Or window move
	// messages, for that matter.)
	constexpr uint32_t flags = (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_RESIZABLE);
	auto *window = SDL_CreateWindow(
		GAME_TITLE, 0, 0, GRP_RES.w, GRP_RES.h, flags
	);
	if(!window) {
		Log_Fail(SDL_LOG_CATEGORY_VIDEO, "Error creating SDL window");
		return 1;
	}
	defer(SDL_DestroyWindow(window));

	SDL_ShowCursor(false);

	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version);
	if(!SDL_GetWindowWMInfo(window, &wminfo)) {
		Log_Fail(SDL_LOG_CATEGORY_VIDEO, "Error retrieving window handles");
		return 1;
	}

	hWndMain = wminfo.info.win.window;
	if(!XInit()) {
		// This is not a SDL error.
		constexpr auto str = (
			"Something went wrong during initialization.\n"
			"Please help fund better error reporting:\n"
			"\n"
			"        https://github.com/nmlgc/ssg/issues/23"
		);
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s", str);
		return 1;
	}
	defer(XCleanup());

	return Run();
}
