/*
 *   Item class
 *
 */

#pragma once

#include "ITEM.H"

class C_ITEM {
private:
	///// [ 変数 ] /////
	std::array<ITEM_DATA, ITEM_MAX> Item;
	std::array<uint16_t, ITEM_MAX> ItemInd;
	uint16_t ItemNow;

public:
	// アイテムを発生させる
	void Set(int x, int y, uint8_t type);

	// アイテムを動かす
	void Move(void);

	// アイテム配列の初期化
	void IndSet(void);

	const ITEM_DATA_CSPAN Data(void) const {
		return Item;
	}

	const std::span<const uint16_t> Inds(void) const {
		return std::span(ItemInd).first(ItemNow);
	}
};


extern C_ITEM Item;
