/*
 *   SDL entry point
 *
 */

#ifdef WIN32
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
#endif

#ifdef SDL3
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#else
#include <SDL.h>
#endif
#include "platform/sdl/sdl2_wrap.h"

#include "GIAN07/ENTRY.H"
#include "platform/window_backend.h"
#include "platform/sdl/log_sdl.h"
#include "game/defer.h"
#include "strings/title.h"
#include "obj/platform_constants.h"
#include "obj/version.h"

#define UTF8_(S) u8 ## S
#define UTF8(S) UTF8_(S)

int main(int argc, char** args)
{
	Log_Init(UTF8(GAME_TITLE));

#ifdef SDL3
#ifndef APP_ID
#define APP_ID "GIAN07"
#endif
	// SDL 3 automatically pulls the Desktop Entry name from the new app
	// metadata, avoiding the need for the environment variable below.
	SDL_SetAppMetadata(GAME_TITLE, VERSION_TAG, APP_ID);
#elif (defined(LINUX) && defined(APP_ID))
	// Tell the Desktop Entry name to the X11 (or Wayland!) window manager, and
	// hope that it uses this name to pick the Desktop Entry's icon.
	//
	// SDL_SetWindowIcon() only allows sending a single icon, which is the
	// wrong approach if you have multiple lovingly hand-crafted variants of
	// your icon at different resolutions. It should be the WM's job to pick
	// the closest available version because we can't possibly know how many
	// pixels it allotted for the icon. SDL_GetWindowBordersSize() is a hack at
	// best because it only tells us the total size of the decorations, not the
	// intended icon size. What should we do if we have a 16-pixel and a
	// 32-pixel icon but get a decoration size of 22?
	//
	/// (Also, SDL_WindowIcon() still works this way even in SDL 3 where
	// `SDL_Surface` gained support for alternate-resolution images.)
	SDL_setenv("SDL_VIDEO_X11_WMCLASS", APP_ID, 1);
#endif

	if(HelpFailed(SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_VIDEO))) {
		Log_Fail(SDL_LOG_CATEGORY_VIDEO, "Error initializing SDL");
		return 1;
	}
	defer(SDL_Quit());

	// The X11 and Wayland backends load their dynamic symbols by trying to
	// look up each function in each of the hardcoded .so files until it's
	// found. This ends up throwing a lot of symbol loading errors at DEBUG
	// level during SDL_VideoInit() that might confuse Linux users, but we'd
	// like this log level for everything we call ourselves. So let's only
	// activate it after SDL_Init().
	SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);

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
