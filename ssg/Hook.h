/*
 *   Hooks for certain game logic events
 *
 */

#pragma once

#include "hatoyama/logic/ffi.h"

// [hooks] is always a valid pointer.
typedef struct HOOKS {
	void *Context;

	// Called for every SCL instruction. [error] is set to `true` if the logic
	// layer failed to process the given opcode.
	void (*SCL_Op)(const uint8_t *cmd, bool8_t error, struct HOOKS *);

	// Called for every sound effect.
	void (*Snd_SEPlay)(uint8_t id, int x, bool8_t loop, struct HOOKS *);
	void (*Snd_SEStop)(uint8_t id, struct HOOKS *);
} HOOKS;

extern HOOKS Hooks;
