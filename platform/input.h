/*
 *   Keyboard and pad input interface
 *
 */

#pragma once

#include "game/input.h"

// 関数プロトタイプ宣言 //
bool Key_Start(void);	// キー入力開始
void Key_End(void);	// キー入力終了

void Key_Read(void);

// Returns:
// • ≥1: ID of the single gamepad button that is being pressed
// •  0: More than one gamepad button is being pressed
// • std::nullopt: No gamepad button is being pressed
std::optional<INPUT_PAD_BUTTON> Key_PadSingle(void);
