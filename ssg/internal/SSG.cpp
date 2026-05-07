/*
 *   Central controller
 *
 */

#include "SSG.hpp"
#include "GIAN.H"
#include "LogicInstance.h"
#include "PRankCtrl.h"
#include "ssg/Hook.h"
#include "ssg/internal/Enemy.hpp"
#include "ssg/internal/LZ.hpp"
#include "ssg/internal/Maid.hpp"
#include "ssg/internal/RNG.hpp"
#include "ssg/internal/Stage.hpp"

constexpr HOOKS HOOKS_EMPTY = {
	.SCL_Op = [](const uint8_t *, bool8_t, HOOKS *) {},
	.ECL_Op = [](const uint8_t *, const ENEMY_DATA *, bool8_t, HOOKS *) {},
	.Snd_SEPlay = [](uint8_t, int, bool8_t, HOOKS *) {},
	.Snd_SEStop = [](uint8_t, HOOKS *) {},
	.Boss_Defeat = [](const BOSS_DATA *, HOOKS *) {},
};

HOOKS Hooks;

C_SSG::C_SSG(void) noexcept : Hooks(::Hooks)
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

bool StageLoad(
	C_SSG& ssg, uint8_t stage, auto&& ecl_buf_func, auto&& scl_buf_func
)
{
	// Re-assigning the `std::unique_ptr`s only frees them one by one, and we
	// want them all freed in any case.
	ssg.StageFree();

	if(!((stage == STAGE_EXTRA) || ((stage >= 1) && (stage <= STAGE_MAX)))) {
		return false;
	}
	if(!(Enemy.Set(ecl_buf_func(), stage) && Stage.Set(scl_buf_func()))) {
		return false;
	}
	return true;
}

bool C_SSG::StageLoadFromDAT(const PACKFILE_READ& enemy_dat, uint8_t stage)
{
	const auto ecl_func = [&] {
		const fil_no_t ecl_no = ((stage == STAGE_EXTRA) ? 24 : (stage + 0 - 1));
		return enemy_dat.MemExpand(ecl_no);
	};
	const auto scl_func = [&] {
		const fil_no_t scl_no = ((stage == STAGE_EXTRA) ? 25 : (stage + 6 - 1));
		return enemy_dat.MemExpand(scl_no);
	};

	return StageLoad(*this, stage, ecl_func, scl_func);
}

void C_SSG::StageFree(void)
{
	// メモリを解放だ！ //
	Enemy.Set(nullptr, 0);
	Stage.Set(nullptr);
}
