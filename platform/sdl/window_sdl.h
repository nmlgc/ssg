/*
 *   SDL window creation
 *
 */

#pragma once

#include <SDL_video.h>
#include "game/graphics.h"

using SDL_DisplayID = int;

// We can't retrieve the original window position in fullscreen mode via
// SDL_GetWindowPosition(), so let's back it up before we go fullscreen.
// Also helpful because these coordinates determine the display that the
// fullscreen window is placed on.
extern std::optional<std::pair<int16_t, int16_t>> TopleftBeforeFullscreen;

// Falls back to the primary display if the window doesn't exist yet.
SDL_DisplayID HelpGetDisplayForWindow(void);

std::pair<int16_t, int16_t> HelpGetWindowPosition(SDL_Window *window);

static constexpr Uint32 HelpFullscreenFlag(const GRAPHICS_FULLSCREEN_FLAGS& fs)
{
	return (!fs.fullscreen ? 0 :
		(fs.exclusive ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_FULLSCREEN_DESKTOP)
	);
}
