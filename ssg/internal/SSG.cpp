/*
 *   Central controller
 *
 */

#include "SSG.hpp"
#include "BOMBEFC.H"
#include "BOSS.H"
#include "EFFECT3D.H"
#include "ENEMY.H"
#include "EnemyExCtrl.h"
#include "FRAGMENT.H"
#include "GIAN.H"
#include "HOMINGL.H"
#include "ITEM.H"
#include "LLASER.H"
#include "LogicInstance.h"
#include "MAID.H"
#include "MAIDTAMA.H"
#include "PRankCtrl.h"
#include "TAMA.H"
#include "ssg/internal/LZ.hpp"
#include "ssg/internal/SEffect.hpp"
#include "ssg/internal/Stage.hpp"

constexpr HOOKS HOOKS_EMPTY = {
	.SCL_Op = [](const uint8_t *, bool8_t, HOOKS *) {},
	.ECL_Op = [](const uint8_t *, const ENEMY_DATA *, bool8_t, HOOKS *) {},
	.Snd_SEPlay = [](uint8_t, int, bool8_t, HOOKS *) {},
	.Snd_SEStop = [](uint8_t, HOOKS *) {},
	.Boss_HPSumAtStart = [](const uint32_t, HOOKS *) {},
	.Boss_Defeat = [](const BOSS_DATA *, HOOKS *) {},
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

	if(
		!Enemy.Set(enemy_dat.MemExpand(ecl_no), stage) ||
		!Stage.Set(enemy_dat.MemExpand(scl_no))
	) {
		return false;
	}

	this->StageInit();
	return true;
}

void C_SSG::StageFree(void)
{
	// メモリを解放だ！ //
	Enemy.Set(nullptr, 0);
	Stage.Set(nullptr);
}

void C_SSG::StageInit(void)
{
	Boss.Init();
	Snaky.Init(); // 蛇管理を初期化(やや謎) //
	Bit.Init(); // ビット管理も初期化 //
	MaidTama.IndSet();
	Enemy.IndSet();
	Tama.IndSet(400 + 200); // 小型弾に４００
	Laser.IndSet();
	LLaser.Setup();
	HLaser.Init();
	SEffect.Init();
	Item.IndSet();
	Fragment.Setup();
	BombEfc.Init();
	Viv.NextStage();
}

void C_SSG::Move(INPUT_BITS input)
{
	Stage.Move(input);
	Effect3D.Move();

	Boss.Move();
	Snaky.Move();
	Bit.Move();
	Enemy.Move();
	Item.Move();
	Tama.Move();
	Laser.Move();
	LLaser.Move();
	HLaser.Move();
	Fragment.Move();
	SEffect.Move();
	BombEfc.Move();

	// この２行の位置を変更しました //
	Viv.Move(input, Stage.InMsg());
	MaidTama.Move();
}
