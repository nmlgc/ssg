/*
 *   Format-independent background music interface
 *
 */

#pragma once

#include <stdint.h>

bool BGM_Init(void);

// Volume control
// --------------

// フェードアウト(数字が大きいほど早い)
void BGM_FadeOut(uint8_t speed);
// --------------

// Tempo control
// -------------

static constexpr uint8_t BGM_TEMPO_DENOM = (1 << 7);	// 標準のテンポ
static constexpr int8_t BGM_TEMPO_MIN = -100;
static constexpr int8_t BGM_TEMPO_MAX =  100;

int8_t BGM_GetTempo(void);
void BGM_SetTempo(int8_t tempo);	// テンポを変更する
// -------------
