/*
 *   Rendering code for game logic entities
 *
 */

#pragma once

#include "ssg/BombEfc.h"
#include "ssg/Boss.h"
#include "ssg/Effect3D.h"
#include "ssg/EnemyExCtrl.h"
#include "ssg/Fragment.h"
#include "ssg/HLaser.h"
#include "ssg/Item.h"
#include "ssg/LLaser.h"
#include "ssg/MaidTama.h"

typedef struct MAID MAID;

// ビット間のラインを描画する
void BitLineDraw(const BIT_DATA&);

// 敵を描画する
void enemy_draw(
	ENEMY_DATA_CSPAN, std::span<const uint16_t> inds, ANIME_DATA_CSPAN
);

// ボスを描画する
void BossDraw(BOSS_DATA_CSPAN, ENEMY_DATA_CSPAN, ANIME_DATA_CSPAN, const MAID&);

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
