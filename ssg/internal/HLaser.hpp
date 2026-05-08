/*
 *   Homing laser class
 *
 */

#pragma once

#include "HOMINGL.H"

struct HOOKS;
class C_VIV;

class C_HLASER {
private:
	HOOKS& Hooks;
	C_VIV& Viv;

	// ホーミングレーザーの本数
	uint16_t HLaserNow;

	// ホーミングレーザー格納バッファ
	HLaserData HLaserBuf[HLASER_MAX];

	// 確保済みホーミングレーザー
	HLaserData ActiveHL;

	// 解放済みホーミングレーザー
	HLaserData FreeHL;

public:
	C_HLASER(HOOKS& Hooks, C_VIV& Viv) noexcept : Hooks(Hooks), Viv(Viv) {
	}

	// ホーミングレーザーの初期化を行う
	void Init(void);

	// ホーミングレーザーをセットする
	void Set(const HLaserInfo *hinfo);

	// ホーミングレーザーを動作させる
	void Move(void);

	// ホーミングレーザーに消去エフェクトをセット
	void Clear(void);

	const HLaserData& Data(void) const {
		return ActiveHL;
	}

	uint16_t NumAlive(void) const {
		return HLaserNow;
	}
};
