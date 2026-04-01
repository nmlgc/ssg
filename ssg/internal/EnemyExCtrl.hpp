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


extern C_SNAKY Snaky;
