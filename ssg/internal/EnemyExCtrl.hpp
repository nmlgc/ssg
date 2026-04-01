/*
 *   Extra enemy control classes
 *
 */

#pragma once

#include "EnemyExCtrl.h"

class C_SNAKY {
private:
	SNAKYMOVE_DATA SnakeData[SNAKE_MAX];

public:
	// 蛇型の敵配列の初期化
	void Init(void);

	// 蛇型の敵をセットする
	void Set(BOSS_DATA *b, int len, uint32_t TailID);

	// 蛇型の敵の移動処理
	void Move(void);

	// 蛇型の敵を殺す
	void Delete(const BOSS_DATA *b);
};


class C_BIT {
private:
	BIT_DATA BitData;

	// 基本的なビット回転処理
	void BitSTDRoll(void);

	// 基本的な半径処理
	void BitSTDRad(void);

public:
	// ビット配列の初期化
	void Init(void);

	// ビットをセットする
	void Set(BOSS_DATA *b, uint8_t NumBits, uint32_t BitID);

	// ビットを動作させる
	void Move(void);

	// ビットを消滅させる
	void Delete(void);

	// 攻撃パターンをセットor変更
	void SelectAttack(uint32_t BitID);

	// レーザー系命令を発行
	void LaserCommand(uint8_t Command);

	// ビット命令を送信
	void SendCommand(uint8_t Command, int Param);

	// 現在のビット数を取得する
	int GetNum(void) const;

	const BIT_DATA& Data(void) const {
		return BitData;
	}
};


extern C_SNAKY Snaky;
extern C_BIT Bit;
