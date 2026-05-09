/*
 *   Static round parameters
 *
 */

#pragma once

#ifdef __cplusplus
import std.compat;
#else
#include <stdint.h>
#endif

// We obviously can't use `constexpr` in FFI headers.
#pragma warning(push)
#pragma warning(disable: 26814)

// 難易度 ＆ オプション設定 //
#define GAME_EASY   	0	// 難易度：Ｅａｓｙ
#define GAME_NORMAL 	1	// 難易度：Ｎｏｒｍａｌ
#define GAME_HARD   	2	// 難易度：Ｈａｒｄ
#define GAME_LUNATIC	3	// 難易度：Ｌｕｎａｔｉｃ
#define GAME_EXTRA  	4	// Ｅｘｔｒａ時...

// Extra Stage starts at Hard.
static const uint8_t EXTRA_LEVEL = GAME_HARD;
static const uint8_t EXTRA_LIVES = 2;

static const uint8_t INPF_MASK = (~0x07);
static const uint8_t INPF_Z_MSKIP_ENABLE   = 0x02; // Ｚキーでメッセージを送れる
static const uint8_t INPF_Z_SPDDOWN_ENABLE = 0x04; // 押しっぱなしでシフト移動

struct ROUND_PARAMS {
	uint32_t Seed;
	uint8_t	LevelSelected;
	uint8_t PlayerStock;
	uint8_t BombStock;
	uint8_t InputFlags;
	uint8_t Weapon;
	uint8_t Exp;
};

#pragma warning(pop)
