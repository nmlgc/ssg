/*
 *   Rendering and management code for the in-game HUD
 *
 */

#pragma once

#include "EFFECT.H"
#include "hatoyama/logic/narrow.h"

struct MAID;

/// BGM title
/// ---------

// Registers the music title rectangle
void MTitleInit(void);

void MTitleSet(WINDOW_COORD y, Narrow::string_view s);
/// ---------

// エフェクトを描画する(仕様変更の可能性があります)
// (Technically game logic!)
void SEffectDraw(SEFFECT_DATA_CSPAN);

// 各種ステータスを描画する
void StateDraw(const MAID&);

void StdStatusOutput(void);
