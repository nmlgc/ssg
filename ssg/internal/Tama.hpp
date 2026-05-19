/*
 *   Bullet class
 *
 */

#pragma once

#include "ssg/Tama.h"
#include "ssg/internal/Entity.hpp"
#include "hatoyama/api/export.h"
#include "hatoyama/logic/coords.h"

class C_FRAGMENT;
class C_ITEM;
class C_PLAYRANK;
class C_RNG;
class C_SEFFECT;
class C_VIV;
struct HOOKS;

class C_TAMA {
public:
	// 標準・弾コマンド構造体
	TAMA_CMD TamaCmd;

private:
	HOOKS& Hooks;
	C_FRAGMENT& Fragment;
	C_ITEM& Item;
	C_PLAYRANK const& PlayRank;
	C_RNG& RNG;
	C_SEFFECT& SEffect;
	C_VIV& Viv;

	////弾の各種変数たち////

	// 小型弾の弾数
	uint16_t Tama1Now;

	// 特殊弾の弾数
	uint16_t Tama2Now;

	// 小型弾の最大数
	uint16_t Tama1Max;

	// 特殊弾の最大数
	uint16_t Tama2Max;

	int TamaSpeed;

	// ホーミング対象のＸ座標
	WORLD_COORD HomingX;

	// ホーミング対象のＹ座標
	WORLD_COORD HomingY;

	// 真ならホーミング実行
	int HomingFlag;

	// 弾の格納用構造体
	std::array<TAMA_DATA, TAMA_MAX> Tama;

	// 小型弾の順番を維持するための配列
	std::array<uint16_t, TAMA_MAX> Tama1Ind;

	// 特殊弾の順番を維持するための配列
	std::array<uint16_t, TAMA_MAX> Tama2Ind;

	// 弾の進行方向をセットする
	uint8_t tama_dir(uint16_t i);

	// 弾の初速度をセットする
	int tama_speed(uint16_t i, int vret, const TAMA_CMD& TamaCmd);

	// 弾のフラグを初期化する
	uint8_t tama_flag(void);

	int LineCmdNewTamaSpeed(uint16_t i);
	int TamaSpeedEx(uint8_t d);

	void __Set(void);

	/// Internal interface, shared between TAMA and MAIDTAMA
	/// ----------------------------------------------------
	friend class C_MAIDTAMA;

	// Initializes an individual (regular) bullet or player shot from `TamaCmd`.
	void SetSingle(
		TAMA_DATA& t,
		unsigned int i,
		int speed,
		decltype(TAMA_DATA::effect) effect
	);

	// 弾の " Type " による移動を行う
	void Tmove(TAMA_DATA *t);

	// 弾の "Option" による移動を行う
	void Omove(TAMA_DATA *t);

	// 弾の "Effect" による移動を行う
	void Emove(TAMA_DATA *t);
	/// ----------------------------------------------------

public:
	////弾関数////

	C_TAMA(
		HOOKS& Hooks,
		C_FRAGMENT& Fragment,
		C_ITEM& Item,
		C_PLAYRANK const& PlayRank,
		C_RNG& RNG,
		C_SEFFECT& SEffect,
		C_VIV& Viv
	) noexcept :
		Hooks(Hooks),
		Fragment(Fragment),
		Item(Item),
		PlayRank(PlayRank),
		RNG(RNG),
		SEffect(SEffect),
		Viv(Viv) {
	}

	// 弾をセットする(難易度による変更は"有り")
	void Set(void);

	// 弾をセットする(難易度による変更は"無し")
	void SetEX(void);

	// 弾をセットする(ライン状に発射)
	void SetLine(void);

	// エキストラボス専用弾幕
	void SetExtra01(void);

	// 弾を動かす
	void Move(void);

	// 弾に消去エフェクトをセットする
	void Clear(void);

	// 弾の個数の割合をセットする(危険！)
	void IndSet(uint16_t tama1);

	// 弾を得点化する(Ret : 得点)
	uint32_t ToScore(void);

	// 弾の 1/n をアイテム化する
	void ToItem(uint8_t n);

	HATOYAMA_API void ResetHoming(void);

	// ホーミング座標を更新する
	void UpdateHoming(const WORLD_POINT& p);

	TAMA_DATA_CSPAN Data(void) const {
		return Tama;
	}

	const std::span<const uint16_t> Inds1(void) const {
		return std::span(Tama1Ind).first(Tama1Now);
	}

	const std::span<const uint16_t> Inds2(void) const {
		return std::span(Tama2Ind).first(Tama2Now);
	}
};


template <size_t N> void Indsort(
	std::array<uint16_t, N>& indices,
	uint16_t& count,
	const std::array<TAMA_DATA, N>& entities
) {
	Indsort(indices, count, entities, [](const TAMA_DATA& t) {
		return (t.flag & TF_DELETE);
	});
}
