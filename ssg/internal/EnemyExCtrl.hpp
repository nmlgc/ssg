/*
 *   Extra enemy control classes
 *
 */

#pragma once

#include "ssg/EnemyExCtrl.h"

class C_ENEMY;
class C_LLASER;
class C_RNG;
class C_VIV;
typedef struct tagBOSS_DATA BOSS_DATA;
typedef struct ENEMY_DATA ENEMY_DATA;
struct HOOKS;

class C_SNAKY {
private:
	C_ENEMY& Enemy;

	SNAKYMOVE_DATA SnakeData[SNAKE_MAX];

public:
	C_SNAKY(C_ENEMY& Enemy) noexcept : Enemy(Enemy) {
	}

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
	HOOKS& Hooks;
	C_ENEMY& Enemy;
	C_LLASER& LLaser;
	C_RNG& RNG;
	C_VIV const& Viv;

	BIT_DATA BitData;

	// 基本的なビット回転処理
	void BitSTDRoll(void);

	// 基本的な半径処理
	void BitSTDRad(void);

public:
	C_BIT(
		HOOKS& Hooks,
		C_ENEMY& Enemy,
		C_LLASER& LLaser,
		C_RNG& RNG,
		C_VIV const& Viv
	) noexcept :
		Hooks(Hooks), Enemy(Enemy), LLaser(LLaser), RNG(RNG), Viv(Viv) {
	}

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
