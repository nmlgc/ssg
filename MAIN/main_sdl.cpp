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
#include "GIAN07/ENTRY.H"
#include "platform/window_backend.h"
#include "platform/sdl/log_sdl.h"
#include "game/defer.h"
#include "strings/title.h"

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

	// Use the backend API's line drawing algorithm, which at least gives us
	// pixel-perfect accuracy with pbg's original 16-bit code when using
	// Direct3D and framebuffer scaling. It does make sense to set this hint
	// unconditionally because the native OpenGL line drawing algorithm is also
	// slightly more accurate to the original game than the SDL algorithm when
	// drawing longer lines. But in any case, we're talking differences of less
	// than 1% of pixels here; both OpenGL and SDL algorithms are at least 97%
	// accurate. For a graphical comparison, see
	//
	// 	https::/rec98.nmlgc.net/blog/2024-10-22#lines-2024-10-22
	//
	// And if we ever want 100% accuracy for every backend API, we can always
	// reimplement Direct3D exact algorithm:
	//
	// 	https://github.com/nmlgc/ssg/issues/74
	SDL_SetHint(SDL_HINT_RENDER_LINE_METHOD, "2");

	// OpenGL is either *the* best or close to the best choice everywhere when
	// it comes to performance (at least as long as we don't batch draw calls):
	//
	// 	https://rec98.nmlgc.net/blog/2024-10-22#benchmark-2024-10-22
	//
	// Yes, this means that players have to manually pick any of the Direct3D
	// APIs to get accurate line drawing, but great performance on everything
	// out of the box is more important.
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

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

	return WndBackend_Run();
}
