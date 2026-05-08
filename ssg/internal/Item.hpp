/*
 *   Item class
 *
 */

#pragma once

#include "ITEM.H"

class C_FRAGMENT;
class C_PLAYRANK;
class C_SEFFECT;
class C_VIV;
struct HOOKS;

class C_ITEM {
private:
	HOOKS& Hooks;
	C_FRAGMENT& Fragment;
	C_PLAYRANK& PlayRank;
	C_SEFFECT& SEffect;
	C_VIV& Viv;

	///// [ 変数 ] /////
	std::array<ITEM_DATA, ITEM_MAX> Item;
	std::array<uint16_t, ITEM_MAX> ItemInd;
	uint16_t ItemNow;

public:
	C_ITEM(
		HOOKS& Hooks,
		C_FRAGMENT& Fragment,
		C_PLAYRANK& PlayRank,
		C_SEFFECT& SEffect,
		C_VIV& Viv
	) noexcept :
		Hooks(Hooks),
		Fragment(Fragment),
		PlayRank(PlayRank),
		SEffect(SEffect),
		Viv(Viv) {
	}

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
