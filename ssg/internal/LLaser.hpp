/*
 *   Long laser class
 *
 */

#pragma once

#include "LLASER.H"

class C_LLASER {
public:
	LLASER_CMD LLaserCmd;

private:
	LLASER_DATA LLaser[LLASER_MAX];

	void _LLaserHitCheck(const LLASER_DATA *lp);

	// レーザーの座標をセットする
	void _LLaserXYSet(int id);

public:
	// レーザーをセットする
	bool Set(uint8_t id);

	// レーザーを開く
	void Open(const ENEMY_DATA *e, uint8_t id);

	// レーザーを閉じる
	void Close(const ENEMY_DATA *e, uint8_t id);

	// レーザーをライン状態にする
	void Line(const ENEMY_DATA *e, uint8_t id);

	// レーザーを角度絶対で回転
	void DegA(const ENEMY_DATA *e, uint8_t d, uint8_t id);

	// レーザーを角度相対で回転);
	void DegR(const ENEMY_DATA *e, char d, uint8_t id);

	// 敵に関連づけられたレーザーを強制クローズ
	void ForceClose(const ENEMY_DATA *e);

	// レーザーを動かす
	void Move(void);

	// 無限遠レーザーを全クローズ
	void Clear(void);

	// レーザー配列の初期化をする
	void Setup(void);

	const LLASER_DATA_CSPAN Data(void) const noexcept {
		return LLaser;
	}
};


extern C_LLASER LLaser;
