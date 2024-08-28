/*
 *   Platform-specific window backend interface
 *
 */

#pragma once

struct GRAPHICS_PARAMS;

// Initialization
// --------------
// Just here to support switchable window backends for certain graphics
// backends.

typedef struct SDL_Window SDL_Window;

// Creates the game window and returns its SDL handle. Fails if the window
// already exists.
SDL_Window *WndBackend_SDLCreate(const GRAPHICS_PARAMS&);

#ifdef WIN32
	#include <windows.h>

	// Returns the Win32 handle of the current window.
	HWND WndBackend_Win32(void);

	// Creates the game window and returns its Win32 handle. Fails if the
	// window already exists.
	HWND WndBackend_Win32Create(const GRAPHICS_PARAMS&);
#endif

void WndBackend_Cleanup(void);
// --------------

// Runs the main loop each frame, and returns the exit code after the game was
// quit.
int WndBackend_Run(void);
