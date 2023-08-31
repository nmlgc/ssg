/*                                                                           */
/*   DI_UTY.h   Direct Input 関連の関数                                      */
/*                                                                           */
/*                                                                           */

#pragma once

#include "game/input.h"
#include <optional>
#include <span>

// 関数プロトタイプ宣言 //
extern bool Key_Start(void);	// キー入力開始
extern void Key_End(void);							// キー入力終了

void Key_Read(const std::span<const INPUT_PAD_BINDING> PadBindings);

// Returns:
// • ≥1: ID of the single gamepad button that is being pressed
// •  0: More than one gamepad button is being pressed
// • std::nullopt: No gamepad button is being pressed
std::optional<INPUT_PAD_BUTTON> Key_PadSingle(void);
