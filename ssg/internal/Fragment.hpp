/*
 *   Particle class
 *
 */

#pragma once

#include "FRAGMENT.H"

class C_FRAGMENT {
private:
	// 次に破片データを挿入する位置
	int FragmentPtr = 0;

	FRAGMENT_DATA Fragment[FRAGMENT_MAX];

public:
	//// 破片関数 ////
	void Set(int x, int y, uint8_t cmd);
	void Move(void);
	void Setup(void);

	FRAGMENT_DATA_CSPAN Data(void) const {
		return Fragment;
	}
};


extern C_FRAGMENT Fragment;
