/*
 *   Central controller
 *
 */

#pragma once

#include "MAID.H"
#include "PRankCtrl.h"
#include "ssg/Round.h"
#include "ssg/internal/RNG.hpp"

struct C_SSG {
	// Starts a new round of gameplay (i.e., a playthrough of either the main 6
	// stages starting at Stage 1, the Extra Stage, or a single stage for
	// practice) by setting [Round] and initializing subsystems as necessary.
	// Sets the number of credits based on [stage_first].
	// Does *not* call `StageInit()`.
	void RoundInit(const ROUND_PARAMS& round, uint8_t stage_first);
};
