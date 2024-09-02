/*
 *   SDL window creation
 *
 */

#pragma once

#include <SDL_video.h>
#include "game/graphics.h"

static constexpr Uint32 HelpFullscreenFlag(const GRAPHICS_FULLSCREEN_FLAGS& fs)
{
	return (fs.fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
}
