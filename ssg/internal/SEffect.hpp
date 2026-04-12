/*
 *   String effect class
 *
 */

#pragma once

#include "ssg/SEffect.h"
#include "hatoyama/logic/coords.h"

class C_SEFFECT {
private:
	SEFFECT_DATA SEffect[SEFFECT_MAX];

public:
	// エフェクトの初期化を行う
	void Init(void);

	// 文字列系エフェクト(上に表示する奴)
	void SetString(int x, int y, const char *s);

	// 得点表示エフェクト
	// (Negative points are very much possible!)
	void SetPoint(WORLD_COORD x, WORLD_COORD y, int32_t point);

	// ゲームオーバーの表示
	void SetGameOver(void);

	// 曲名の表示
	void SetMTitle(WINDOW_COORD y, const PIXEL_SIZE& extent);

	// エフェクトを動かす(仕様変更の可能性があります)
	void Move(void);

	SEFFECT_DATA_CSPAN Data(void) const {
		return SEffect;
	}
};


extern C_SEFFECT SEffect;
