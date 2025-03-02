/*
 *   Platform-specific window backend interface
 *
 */

#pragma once

import std.compat;
struct GRAPHICS_PARAMS;

// Initialization
// --------------
// Just here to support switchable window backends for certain graphics
// backends.

typedef struct SDL_Window SDL_Window;

// Looks like it belongs into `graphics_sdl`, but is also needed for window
// creation.
std::u8string_view WndBackend_SDLRendererName(int8_t id);

// Returns the SDL handle of the current window.
SDL_Window *WndBackend_SDL(void);

// Creates the game window and returns its SDL handle. Fails if the window
// already exists.
SDL_Window *WndBackend_SDLCreate(const GRAPHICS_PARAMS&);

#ifdef WIN32_VINTAGE
#include <windows.h>

// Returns the Win32 handle of the current window.
HWND WndBackend_Win32(void);

// Creates the game window and returns its Win32 handle. Fails if the window
// already exists.
HWND WndBackend_Win32Create(const GRAPHICS_PARAMS&);
#endif

void WndBackend_Cleanup(void);
// --------------

// Returns the current top-left position of the game window.
std::optional<std::pair<int16_t, int16_t>> WndBackend_Topleft(void);

// Runs the main loop each frame, and returns the exit code after the game was
// quit.
int WndBackend_Run(void);
