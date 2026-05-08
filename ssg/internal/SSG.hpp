/*
 *   Central controller
 *
 */

#pragma once

#include "ssg/Input.h"
#include "ssg/internal/BombEfc.hpp"
#include "ssg/internal/Boss.hpp"
#include "ssg/internal/Effect3D.hpp"
#include "ssg/internal/Enemy.hpp"
#include "ssg/internal/EnemyExCtrl.hpp"
#include "ssg/internal/Fragment.hpp"
#include "ssg/internal/HLaser.hpp"
#include "ssg/internal/Item.hpp"
#include "ssg/internal/LLaser.hpp"
#include "ssg/internal/Laser.hpp"
#include "ssg/internal/Maid.hpp"
#include "ssg/internal/MaidTama.hpp"
#include "ssg/internal/RNG.hpp"
#include "ssg/internal/SEffect.hpp"
#include "ssg/internal/Stage.hpp"
#include "ssg/internal/Tama.hpp"

struct HOOKS;
struct PACKFILE_READ;
struct ROUND_PARAMS;

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
};
