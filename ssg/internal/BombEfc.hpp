/*
 *   Bomb effect class
 *
 */

#pragma once

#include "BOMBEFC.H"

class C_RNG;

class C_BOMBEFC {
private:
	C_RNG& RNG;

	BombEfcCtrl BombEfc[EXBOMB_MAX];

public:
	C_BOMBEFC(C_RNG& RNG) noexcept : RNG(RNG) {
	}

	// 爆発系エフェクトの初期化
	void Init(void);

	// 爆発系エフェクトをセットする
	void Set(int x, int y, uint8_t type);

	// 爆発系エフェクトを動作させる
	void Move(void);

	BOMBEFC_DATA_CSPAN Data(void) const {
		return BombEfc;
	}
};
