/*
 *   Player class
 *
 */

#pragma once

#include "ssg/Maid.h"
#include "ssg/Input.h"

class C_FRAGMENT;
class C_LASER;
class C_MAIDTAMA;
class C_PLAYRANK;
class C_SEFFECT;
class C_TAMA;
struct HOOKS;
struct ROUND_PARAMS;

// 麗しきメイドさん構造体
class C_VIV : public MAID {
private:
	HOOKS& Hooks;
	const ROUND_PARAMS& Round;
	C_FRAGMENT& Fragment;
	C_LASER& Laser;
	C_MAIDTAMA& MaidTama;
	C_PLAYRANK& PlayRank;
	C_SEFFECT& SEffect;
	C_TAMA& Tama;

	/// Internal interface
	/// ------------------
	friend class C_SSG;

	// 初期化
	void Set(uint8_t stage_first);
	/// ------------------

public:
	C_VIV(
		HOOKS& Hooks,
		const ROUND_PARAMS& Round,
		C_FRAGMENT& Fragment,
		C_LASER& Laser,
		C_MAIDTAMA& MaidTama,
		C_PLAYRANK& PlayRank,
		C_SEFFECT& SEffect,
		C_TAMA& Tama
	) noexcept :
		Hooks(Hooks),
		Round(Round),
		Fragment(Fragment),
		Laser(Laser),
		MaidTama(MaidTama),
		PlayRank(PlayRank),
		SEffect(SEffect),
		Tama(Tama) {
	}

	///// [ 関数 ] /////
	void Move(INPUT_BITS, bool in_msg);

	// 次のステージの準備
	void NextStage(void);

	// 死す
	void Dead(void);

	// コンティニューを行う場合
	bool Continue(void);

	// かすりゲージを上昇させる
	void evade_add(uint8_t n);
	void evade_addEx(int x, int y, uint8_t n);

	// スコアを上昇させる
	void score_add(int sc);

	void PowerUp(uint8_t damage);
};
