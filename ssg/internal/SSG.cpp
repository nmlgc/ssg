/*
 *   Central controller
 *
 */

#include "ssg/internal/SSG.hpp"
#include "ssg/Gian.h"
#include "ssg/internal/LZ.hpp"

constexpr HOOKS HOOKS_EMPTY = {
	.SCL_Op = [](const uint8_t *, bool8_t, HOOKS *) {},
	.ECL_Op = [](const uint8_t *, const ENEMY_DATA *, bool8_t, HOOKS *) {},
	.Snd_SEPlay = [](uint8_t, int, bool8_t, HOOKS *) {},
	.Snd_SEStop = [](uint8_t, HOOKS *) {},
	.Boss_HPSumAtStart = [](const uint32_t, HOOKS *) {},
	.Boss_Defeat = [](const BOSS_DATA *, HOOKS *) {},
	.GameOver = [](HOOKS *) {},
};

C_SSG::C_SSG(void) noexcept :
	Hooks(HOOKS_EMPTY),
	Bit(Hooks, Enemy, LLaser, RNG, Viv),
	BombEfc(RNG),
	Boss(
		Hooks,
		Bit,
		BombEfc,
		Enemy,
		Fragment,
		Item,
		Laser,
		SEffect,
		Snaky,
		Tama,
		Viv
	),
	Effect3D(RNG),
	Enemy(
		Hooks,
		Boss,
		Effect3D,
		HLaser,
		Item,
		Laser,
		LLaser,
		PlayRank,
		RNG,
		Tama,
		Viv
	),
	Fragment(RNG),
	HLaser(Hooks, Viv),
	Item(Hooks, Fragment, PlayRank, SEffect, Viv),
	Laser(LLaser, PlayRank, RNG, Viv),
	LLaser(Hooks, Viv),
	MaidTama(Hooks, Enemy, Fragment, PlayRank, Tama, Viv),
	PlayRank(Stage, Round),
	SEffect(RNG),
	Snaky(Enemy),
	Stage(Hooks, Round, Boss, Effect3D, Enemy, PlayRank),
	Tama(Hooks, Fragment, Item, PlayRank, RNG, SEffect, Viv),
	Viv(Hooks, Round, Fragment, Laser, MaidTama, PlayRank, SEffect, Tama)
{
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

void C_SSG::RoundContinue(void)
{
	Viv.Continue();
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
		!Stage.Set(enemy_dat.MemExpand(scl_no), stage)
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
	Stage.Set(nullptr, 0);
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
	assert(Viv.GameOverTimer == 0);

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

unsigned int C_SSG::MoveGameOver(void)
{
	if(Viv.GameOverTimer <= 0) {
		return 0;
	}

	Viv.GameOverTimer--;
	Fragment.Move();
	SEffect.Move();
	return Viv.GameOverTimer;
}
