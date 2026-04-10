/*
 *   Rendering and management code for the in-game HUD
 *
 */

#pragma once

#include "hatoyama/logic/coords.h"
#include "hatoyama/logic/narrow.h"

/// BGM title
/// ---------

// Registers the music title rectangle
void MTitleInit(void);

void MTitleSet(WINDOW_COORD y, Narrow::string_view s);

// (temporary, SEffectDraw() will be moved into the same TU later)
void MTitleRender(WINDOW_POINT topleft, const PIXEL_LTWH& subrect);
/// ---------

// 各種ステータスを描画する
void StateDraw(void);

void StdStatusOutput(void);
