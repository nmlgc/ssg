/*
 *   Bomb effect class
 *
 */

#pragma once

#include "BOMBEFC.H"

class C_BOMBEFC {
private:
	BombEfcCtrl BombEfc[EXBOMB_MAX];

public:
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


extern C_BOMBEFC BombEfc;
