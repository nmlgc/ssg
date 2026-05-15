/*
 *   Central controller
 *
 */

#pragma once

#include "ssg/Hook.h"
#include "ssg/Input.h"
#include "ssg/Round.h"
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
#include "ssg/internal/PRankCtrl.hpp"
#include "ssg/internal/RNG.hpp"
#include "ssg/internal/SEffect.hpp"
#include "ssg/internal/Stage.hpp"

struct PACKFILE_READ;

struct C_SSG {
	HOOKS Hooks;
	ROUND_PARAMS Round;
	C_BIT Bit;
	C_BOMBEFC BombEfc;
	C_BOSS Boss;
	C_EFFECT3D Effect3D;
	C_ENEMY Enemy;
	C_FRAGMENT Fragment;
	C_HLASER HLaser;
	C_ITEM Item;
	C_LASER Laser;
	C_LLASER LLaser;
	C_MAIDTAMA MaidTama;
	C_PLAYRANK PlayRank;
	C_RNG RNG;
	C_SEFFECT SEffect;
	C_SNAKY Snaky;
	C_STAGE Stage;
	C_TAMA Tama;
	C_VIV Viv;

#ifdef _MSC_VER
	// Prevent compile-time initialization
	__declspec(noinline)
#endif
	C_SSG(void) noexcept;
	C_SSG(C_SSG&&) = delete;

	// Starts a new round of gameplay (i.e., a playthrough of either the main 6
	// stages starting at Stage 1, the Extra Stage, or a single stage for
	// practice) by setting [Round] and initializing subsystems as necessary.
	// Sets the number of credits based on [stage_first].
	// Does *not* call `StageInit()`.
	void RoundInit(const ROUND_PARAMS& round, uint8_t stage_first);

	// Moves from the Game Over state back to the regular game state by using a
	// continue.
	void RoundContinue(void);

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
