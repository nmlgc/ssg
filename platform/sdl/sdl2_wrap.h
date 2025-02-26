/*
 *   SDL 2 compatibility wrappers
 *
 *   This backend is primarily written against SDL 3. This header abstracts
 *   away some of the changes to make it compile with SDL 2 – the inverse of
 *   sdl2-compat, so to speak.
 */

#pragma once

#ifdef SDL3
inline constexpr bool HelpFailed(bool sdl_ret)
{
	return !sdl_ret;
}
#else
inline constexpr bool HelpFailed(int sdl_ret)
{
	return (sdl_ret != 0);
}

// Yeah, `using` would be cleaner, but it rightfully refuses to override the
// `SDL_PixelFormat` struct definition from SDL 2...
#define SDL_PixelFormat Uint32

using SDL_DisplayID = int;

// Renamed constants
// -----------------

#define SDL_EVENT_FIRST                SDL_FIRSTEVENT
#define SDL_EVENT_JOYSTICK_ADDED       SDL_JOYDEVICEADDED
#define SDL_EVENT_JOYSTICK_AXIS_MOTION SDL_JOYAXISMOTION
#define SDL_EVENT_JOYSTICK_BUTTON_DOWN SDL_JOYBUTTONDOWN
#define SDL_EVENT_JOYSTICK_BUTTON_UP   SDL_JOYBUTTONUP
#define SDL_EVENT_JOYSTICK_REMOVED     SDL_JOYDEVICEREMOVED
#define SDL_EVENT_KEY_DOWN             SDL_KEYDOWN
#define SDL_EVENT_KEY_UP               SDL_KEYUP
#define SDL_EVENT_LAST                 SDL_LASTEVENT
#define SDL_EVENT_QUIT                 SDL_QUIT
#define SDL_EVENT_WINDOW_FOCUS_GAINED  SDL_WINDOWEVENT_FOCUS_GAINED
#define SDL_EVENT_WINDOW_FOCUS_LOST    SDL_WINDOWEVENT_FOCUS_LOST
#define SDL_KMOD_LALT                  KMOD_LALT
#define SDL_KMOD_SCROLL                KMOD_SCROLL
#define SDL_SCALEMODE_LINEAR           SDL_ScaleModeBest
#define SDL_SCALEMODE_NEAREST          SDL_ScaleModeNearest
#define SDL_SCANCODE_COUNT             SDL_NUM_SCANCODES
// -----------------

inline SDL_Surface *SDL_CreateSurface(int w, int h, SDL_PixelFormat format)
{
	return SDL_CreateRGBSurfaceWithFormat(0, w, h, 0, format);
}

// Renamed functions
// -----------------

#define SDL_CloseGamepad         SDL_GameControllerClose
#define SDL_CloseJoystick        SDL_JoystickClose
#define SDL_DestroySurface       SDL_FreeSurface
#define SDL_FlushRenderer        SDL_RenderFlush
#define SDL_GetJoystickFromID    SDL_JoystickFromInstanceID
#define SDL_GetLogOutputFunction SDL_LogGetOutputFunction
#define SDL_GetRenderScale       SDL_RenderGetScale
#define SDL_GetTicks             SDL_GetTicks64
#define SDL_IOFromMem            SDL_RWFromMem
#define SDL_IsGamepad            SDL_IsGameController
#define SDL_LoadBMP_IO           SDL_LoadBMP_RW
#define SDL_OpenGamepad          SDL_GameControllerOpen
#define SDL_OpenJoystick         SDL_JoystickOpen
#define SDL_RenderFillRect       SDL_RenderFillRectF
#define SDL_RenderLine           SDL_RenderDrawLine
#define SDL_RenderLines          SDL_RenderDrawLinesF
#define SDL_RenderTexture        SDL_RenderCopy
#define SDL_SetLogOutputFunction SDL_LogSetOutputFunction
#define SDL_SetLogPriorities     SDL_LogSetAllPriority
#define SDL_SetRenderClipRect    SDL_RenderSetClipRect
#define SDL_SetSurfaceColorKey   SDL_SetColorKey
// -----------------
#endif
