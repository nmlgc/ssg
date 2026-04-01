/*
 *   Rendering code for game logic entities
 *
 */

#pragma once

#include "BOMBEFC.H"
#include "BOSS.H"
#include "EFFECT3D.H"
#include "ENEMY.H"
#include "FRAGMENT.H"
#include "HOMINGL.H"
#include "ITEM.H"
#include "LASER.H"
#include "LLASER.H"
#include "MAIDTAMA.H"

class C_BIT;

// ビット間のラインを描画する
void BitLineDraw(const C_BIT&);

// 敵を描画する
void enemy_draw(ENEMY_DATA_CSPAN storage, std::span<const uint16_t> inds);

// ボスを描画する
void BossDraw(BOSS_DATA_CSPAN Boss, const MAID&);

// 爆発系エフェクトを描画する
void ExBombEfcDraw(BOMBEFC_DATA_CSPAN);

// ３面高速星描画
void DrawStg3Star(STG6STAR_CSPAN);

void DrawStg4Rock(ROCK3D_CSPAN);

// ６面ラスター描画
void DrawStg6Raster(STG6STAR_CSPAN, STG6RASTER_CSPAN);

void Draw3DCube(STAR2D_CSPAN, CUBE3D_CSPAN);
void DrawEffectFakeECL(const WFLine2D&, FAKE_ECLSTR_CSPAN);

void fragment_draw(FRAGMENT_DATA_CSPAN);

// ホーミングレーザーを描画する
void HLaserDraw(const HLaserData& ActiveHL);

// アイテムを描画する
void ItemDraw(ITEM_DATA_CSPAN storage, std::span<const uint16_t> inds);

// レーザーを描画する
void laser_draw(LASER_DATA_CSPAN storage, std::span<const uint16_t> inds);

// レーザーを描画する
void LLaserDraw(LLASER_DATA_CSPAN llaser);

void MaidDraw(const MAID&);
void StateDraw(const MAID&);    // 各種ステータスを描画する
void WideBombDraw(const MAID&); // ワイドショット用のボム(やや例外処理)

// ナニな弾描画
void MaidTamaDraw(
	MAIDTAMA_DATA_CSPAN storage, std::span<const uint16_t> inds, const MAID&
);

// 弾を描画する
void tama_draw(
	TAMA_DATA_CSPAN storage,
	std::span<const uint16_t> inds1,
	std::span<const uint16_t> inds2
);
