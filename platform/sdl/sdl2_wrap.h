/*
 *   SDL 2 compatibility wrappers
 *
 */

#pragma once

// Yeah, `using` would be cleaner, but it rightfully refuses to override the
// `SDL_PixelFormat` struct definition from SDL 2...
#define SDL_PixelFormat Uint32
