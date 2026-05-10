/*
 *   String effects
 *
 *   (Game logic because [SEFC_STR1_2] uses the logic's RNG)
 */

#pragma once

import std.compat;
#include "hatoyama/logic/coords.h"

///// [ 定数 ] /////
#define SEFFECT_MAX 1000

#define SEFC_NONE   0x00 // 未使用
#define SEFC_STR1   0x01 // 文字列エフェクト１
#define SEFC_STR1_2 0x02 // 文字列一時停止
#define SEFC_STR1_3 0x03 // 文字列爆発

#define SEFC_MTITLE1 0x04 // 曲名表示エフェクト(出動)
#define SEFC_MTITLE2 0x05 // 曲名表示エフェクト(停止)
#define SEFC_MTITLE3 0x06 // 曲名表示エフェクト(退却)

#define SEFC_GAMEOVER  0x07 // ワーニングの表示とか
#define SEFC_GAMEOVER2 0x08 // ワーニングの表示とか

#define SEFC_STR2 0x10 // 得点アイテム用？エフェクト



///// [構造体] /////
struct SEFFECT_DATA {
	int		x,y;
	int		vx,vy;

	uint32_t	time;

	// (Negative points are very much possible!)
	int32_t point;

	uint8_t	cmd;
	char	c;
};

using SEFFECT_DATA_CSPAN = std::span<const SEFFECT_DATA, SEFFECT_MAX>;



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
