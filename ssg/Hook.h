/*
 *   Hooks for certain game logic events
 *
 */

#pragma once

#include "hatoyama/logic/ffi.h"

typedef struct BOSS_DATA BOSS_DATA;
typedef struct ENEMY_DATA ENEMY_DATA;

// [hooks] is always a valid pointer.
typedef struct HOOKS {
	void *Context;

	// Called for every SCL instruction. [error] is set to `true` if the logic
	// layer failed to process the given opcode.
	void (*SCL_Op)(const uint8_t *cmd, bool8_t error, struct HOOKS *);

	// Called after every ECL instruction whose [ecl_hook.flag] is `true`.
	// [error] is set to `true` if the logic layer failed to process the given
	// opcode; in that case, [ecl_hook.flag] is bypassed.
	void (*ECL_Op)(
		const uint8_t *cmd, const ENEMY_DATA *e, bool8_t error, struct HOOKS *
	);

	// Called for every sound effect.
	void (*Snd_SEPlay)(uint8_t id, int x, bool8_t loop, struct HOOKS *);
	void (*Snd_SEStop)(uint8_t id, struct HOOKS *);

	// Called every frame with the HP sum of all bosses at the beginning of the
	// frame, before collision detection.
	void (*Boss_HPSumAtStart)(const uint32_t hp_sum, struct HOOKS *);

	// Called when defeating the given boss.
	void (*Boss_Defeat)(const BOSS_DATA *b, struct HOOKS *);

	// Called on a Game Over.
	void (*GameOver)(struct HOOKS *);

	// Indexed with the opcodes in ECL.H, this array activates the [ECL_Op]
	// hook for the respective opcode if nonzero.
	bool8_t ecl_hook_flag[256];
} HOOKS;

extern HOOKS Hooks;
