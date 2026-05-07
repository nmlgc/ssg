/*
 *   Central controller
 *
 */

#include "SSG.hpp"
#include "ENEMY.H"
#include "GIAN.H"
#include "LogicInstance.h"
#include "ssg/internal/LZ.hpp"
#include "ssg/internal/Stage.hpp"

constexpr HOOKS HOOKS_EMPTY = {
	.SCL_Op = [](const uint8_t *, bool8_t, HOOKS *) {},
	.ECL_Op = [](const uint8_t *, const ENEMY_DATA *, bool8_t, HOOKS *) {},
	.Snd_SEPlay = [](uint8_t, int, bool8_t, HOOKS *) {},
	.Snd_SEStop = [](uint8_t, HOOKS *) {},
};

HOOKS Hooks;

C_SSG::C_SSG(void) noexcept : Hooks(Hooks)
{
	Hooks = HOOKS_EMPTY;
}

void C_SSG::RoundInit(const ROUND_PARAMS& round, uint8_t stage_first)
{
	Round = round;
	Viv.Set(stage_first);
	PlayRank.Reset();

	// 乱数の初期化 //
	// 最後に乱数もそろえる //
	RNG.seed = round.Seed;
}

bool C_SSG::StageLoadFromDAT(const PACKFILE_READ& enemy_dat, uint8_t stage)
{
	// Re-assigning the `std::unique_ptr`s only frees them one by one, and we
	// want them all freed in any case.
	this->StageFree();

	if(!((stage == STAGE_EXTRA) || ((stage >= 1) || (stage <= STAGE_MAX)))) {
		return true;
	}

	const fil_no_t ecl_no = ((stage == STAGE_EXTRA) ? 24 : (stage + 0 - 1));
	const fil_no_t scl_no = ((stage == STAGE_EXTRA) ? 25 : (stage + 6 - 1));

	return (
		Enemy.Set(enemy_dat.MemExpand(ecl_no), stage) &&
		Stage.Set(enemy_dat.MemExpand(scl_no))
	);
}

void C_SSG::StageFree(void)
{
	// メモリを解放だ！ //
	Enemy.Set(nullptr, 0);
	Stage.Set(nullptr);
}
