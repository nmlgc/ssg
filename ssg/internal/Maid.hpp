/*
 *   Player class
 *
 */

#pragma once

#include "MAID.H"
#include "ssg/Input.h"

// 麗しきメイドさん構造体
class C_VIV : public MAID {
private:
	/// Internal interface
	/// ------------------
	friend class C_SSG;

	// 初期化
	void Set(uint8_t stage_first);
	/// ------------------

public:
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


extern C_VIV Viv;
