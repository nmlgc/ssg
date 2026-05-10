/*
 *   Central controller
 *
 */

#pragma once

#include "MAID.H"
#include "PRankCtrl.h"
#include "ssg/Round.h"
#include "ssg/internal/RNG.hpp"
#include "ssg/Hook.h"
#include "ssg/Input.h"

struct PACKFILE_READ;

struct C_SSG {
	HOOKS& Hooks;

	C_SSG(void) noexcept;

	// Starts a new round of gameplay (i.e., a playthrough of either the main 6
	// stages starting at Stage 1, the Extra Stage, or a single stage for
	// practice) by setting [Round] and initializing subsystems as necessary.
	// Sets the number of credits based on [stage_first].
	// Does *not* call `StageInit()`.
	void RoundInit(const ROUND_PARAMS& round, uint8_t stage_first);

	// Calls `StageFree()`, then initializes the enemy and SCL subsystems with
	// the data for the given [stage], decompressed from a previously
	// initialized packfile instance of `ENEMY.DAT`. Returns `true` on success.
	bool StageLoadFromDAT(const PACKFILE_READ& enemy_dat, uint8_t stage);

	// Deallocates enemy and SCL data.
	void StageFree(void);

	// Initializes all subsystems for a new stage.
	void StageInit(void);

	// Updates a single game frame, using the given [input].
	void Move(INPUT_BITS input);

	// Updates a single game frame during a Game Over state.
	// Returns the number of frames remaining in the Game Over animation, and
	// *must* be called until it returns 0 to correctly advance RNG state after
	// a continue.
	unsigned int MoveGameOver(void);
};
