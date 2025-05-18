/*
 *   難易度 ＆ オプション設定
 *
 */

#pragma once

#define GAME_EASY   	0	// 難易度：Ｅａｓｙ
#define GAME_NORMAL 	1	// 難易度：Ｎｏｒｍａｌ
#define GAME_HARD   	2	// 難易度：Ｈａｒｄ
#define GAME_LUNATIC	3	// 難易度：Ｌｕｎａｔｉｃ
#define GAME_EXTRA  	4	// Ｅｘｔｒａ時...

// Extra Stage starts at Hard.
static constexpr auto EXTRA_LEVEL = GAME_HARD;
static constexpr auto EXTRA_LIVES = 2;
