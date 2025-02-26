/*
 *   Internal logging helpers for the SDL platform
 *
 */

#pragma once

import std;

#ifdef SDL3
#include <SDL3/SDL_log.h>
#else
#include <SDL_log.h>
#endif

void Log_Init(const char8_t *title);

// Logs [msg] together with the last SDL_GetError() as a critical error.
void Log_Fail(SDL_LogCategory category, const char *msg);
