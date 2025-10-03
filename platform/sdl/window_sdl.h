/*
 *   SDL window creation
 *
 */

#pragma once

#include <SDL3/SDL_video.h>

#include "game/graphics.h"

// We can't retrieve the original window position in fullscreen mode via
// SDL_GetWindowPosition(), so let's back it up before we go fullscreen.
// Also helpful because these coordinates determine the display that the
// fullscreen window is placed on.
extern std::optional<std::pair<int16_t, int16_t>> TopleftBeforeFullscreen;

// Falls back to the primary display if the window doesn't exist yet.
SDL_DisplayID HelpGetDisplayForWindow(void);

std::pair<int16_t, int16_t> HelpGetWindowPosition(SDL_Window *window);

// Returns the SDL render driver index that matches [hint]. If the hint doesn't
// match any driver, the function resets SDL's render driver hints and returns
// -1.
int8_t WndBackend_ValidateRenderDriver(const std::u8string_view hint);

// Looks like it belongs into `graphics_sdl`, but is also needed for window
// creation.
std::u8string_view WndBackend_SDLRendererName(int8_t id);

// Returns the new active fullscreen flags if the mode change was successful.
[[nodiscard]] std::optional<GRAPHICS_FULLSCREEN_FLAGS> HelpSetFullscreenMode(
	SDL_Window *window, GRAPHICS_FULLSCREEN_FLAGS fs
);
