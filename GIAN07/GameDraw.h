/*
 *   Rendering code for game logic entities
 *
 */

#pragma once

#include "LLASER.H"
#include "MAIDTAMA.H"

class C_BIT;
class C_BOSS;
class C_BOMBEFC;
class C_EFFECT3D;
class C_ENEMY;
class C_FRAGMENT;
class C_HLASER;
class C_ITEM;
class C_LASER;

// ビット間のラインを描画する
void BitLineDraw(const C_BIT&);

// 敵を描画する
void enemy_draw(const C_ENEMY&);

// ボスを描画する
void BossDraw(const C_BOSS&, const C_ENEMY&, const MAID&);

// 爆発系エフェクトを描画する
void ExBombEfcDraw(const C_BOMBEFC&);

// ３面高速星描画
void DrawStg3Star(const C_EFFECT3D&);

void DrawStg4Rock(const C_EFFECT3D&);

// ６面ラスター描画
void DrawStg6Raster(const C_EFFECT3D&);

void Draw3DCube(const C_EFFECT3D&);
void DrawEffectFakeECL(const C_EFFECT3D&);

void fragment_draw(const C_FRAGMENT&);

// ホーミングレーザーを描画する
void HLaserDraw(const C_HLASER&);

// アイテムを描画する
void ItemDraw(const C_ITEM&);

// レーザーを描画する
void laser_draw(const C_LASER&);

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
