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

// Returns the SDL handle of the current window.
SDL_Window *WndBackend_SDL(void);

// Creates the game window and returns the actual configuration the backend is
// running. Fails if the window already exists.
std::optional<GRAPHICS_PARAMS> WndBackend_Create(GRAPHICS_PARAMS);

#ifdef WIN32_VINTAGE
#include <windows.h>

// Returns the Win32 handle of the current window.
HWND WndBackend_Win32(void);
#endif

void WndBackend_Cleanup(void);
// --------------

// Returns the current top-left position of the game window.
std::optional<std::pair<int16_t, int16_t>> WndBackend_Topleft(void);

// Runs the main loop each frame, and returns the exit code after the game was
// quit.
int WndBackend_Run(void);
