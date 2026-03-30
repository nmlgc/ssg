/*
 *   Boss HP gauge
 *
 */

#pragma once

import std.compat;

void BossHPG_Init(void);
void BossHPG_Open(uint32_t max);	// ボスの体力ゲージをオープンする
void BossHPG_Move(uint32_t now);	// ボスの体力ゲージを増減する
void BossHPG_Close(void);	// ボスの体力ゲージをクローズする
void BossHPG_Update(uint32_t next);	// ボスの体力ゲージを上昇させる
void BossHPG_Draw(void);	// ボスの体力ゲージを描画する
