/*
 *   SDL window creation
 *
 */

#pragma once

#include <SDL_video.h>
#include "game/graphics.h"

static constexpr Uint32 HelpFullscreenFlag(const GRAPHICS_FULLSCREEN_FLAGS& fs)
{
	return (!fs.fullscreen ? 0 :
		(fs.exclusive ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_FULLSCREEN_DESKTOP)
	);
}
