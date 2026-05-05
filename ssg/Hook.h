/*
 *   Hooks for certain game logic events
 *
 */

#pragma once

#include "hatoyama/logic/ffi.h"

// [hooks] is always a valid pointer.
typedef struct HOOKS {
	void *Context;

	// Called for every sound effect.
	void (*Snd_SEPlay)(uint8_t id, int x, bool8_t loop, struct HOOKS *);
	void (*Snd_SEStop)(uint8_t id, struct HOOKS *);
} HOOKS;

extern HOOKS Hooks;
