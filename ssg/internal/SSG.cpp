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

void C_SSG::RoundInit(const ROUND_PARAMS& round, uint8_t stage_first)
{
	Round = round;
	MaidSet(stage_first);
	PlayRankReset();

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
		StageSet(enemy_dat.MemExpand(scl_no))
	);
}

void C_SSG::StageFree(void)
{
	// メモリを解放だ！ //
	Enemy.Set(nullptr, 0);
	StageSet(nullptr);
}
