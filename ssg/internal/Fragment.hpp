/*
 *   Particle class
 *
 */

#pragma once

#include "ssg/Fragment.h"

class C_RNG;

class C_FRAGMENT {
private:
	C_RNG& RNG;

	// 次に破片データを挿入する位置
	int FragmentPtr = 0;

	FRAGMENT_DATA Fragment[FRAGMENT_MAX];

public:
	C_FRAGMENT(C_RNG& RNG) noexcept : RNG(RNG) {
	}

	//// 破片関数 ////
	void Set(int x, int y, uint8_t cmd);
	void Move(void);
	void Setup(void);

	FRAGMENT_DATA_CSPAN Data(void) const {
		return Fragment;
	}
};
