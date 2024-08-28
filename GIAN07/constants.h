/*
 *   Game-specific compile-time constants and types to be shared with the
 *   platform layers
 *
 */

#pragma once

import std.compat;
#include "game/coords.h"
#include "strings/title.h"
#include <assert.h>

// The game's native resolution.
constexpr WINDOW_SIZE GRP_RES      = { 640, 480 };
constexpr WINDOW_LTRB GRP_RES_RECT = { { 0, 0 }, GRP_RES };

// Yup, the game is supposed to be running at 62.5 FPS.
constexpr auto FRAME_TIME_TARGET = 16;

// ID types
// --------

constexpr auto FACE_MAX = 3; // 同時にロード可能な人数...
constexpr auto ENDING_PIC_MAX = 6;

enum class SURFACE_ID : uint8_t {
	SYSTEM,	// システム用

	// Title Screen
	TITLE = 2,	// たいとる用

	// Music Room
	MUSIC = 2,	// 音楽室用

	// Name Registration
	NAMEREG = 2,	// お名前登録用

	// In-game
	MAPCHIP = 1,	// 背景用
	ENEMY = 2,	// 敵(雑魚＆ボス)用
	FACE,	// 顔グラ用
	FACE_last = (FACE + FACE_MAX - 1),
	BOMBER,	// ボム用グラフィック用

	// Splash screen
	SPROJECT = 1,	// 西方Project表示用

	// Endings
	ENDING_CREDITS = 1,
	ENDING_PIC,
	ENDING_PIC_last = (ENDING_PIC + ENDING_PIC_MAX - 1),

	// Rendered text. Since this one is procedurally generated and therefore
	// doesn't have a palette, it must come last to ensure that DirectDraw
	// initializes it with the implicit palette loaded for an earlier surface.
	TEXT,

	COUNT,
};

// Addition is only defined for the types of surfaces we have multiple of.
static constexpr SURFACE_ID operator+(SURFACE_ID lhs, uint8_t rhs)
{
	assert(!((lhs == SURFACE_ID::FACE) && (rhs >= FACE_MAX)));
	assert(!((lhs == SURFACE_ID::ENDING_PIC) && (rhs >= ENDING_PIC_MAX)));
	return SURFACE_ID{ static_cast<uint8_t>(std::to_underlying(lhs) + rhs) };
}

enum class FONT_ID : uint8_t {
	SMALL = 0,	// フォント(小さい文字用)
	NORMAL = 1,	// フォント(通常の文字用)
	LARGE = 2,	// フォント(大きい文字用)
	COUNT,
};
// --------

// Mapping world coordinates to a position in the stereo field
// -----------------------------------------------------------
// These constants map the [x] parameter from any source unit to a position in
// the stereo field. The resulting unit is the attenuation volume of either the
// right (negative) or left (positive) channel, expressed in decibels.
// The algorithm from the original game:
//
// • The [x] values are world coordinates (Q26.6, 64 units per pixel)
// • Subtract the center of the screen (in world coordinates) from [x]
// • Divide the result by (a scalar) 16
// • Directly pass that result to DirectSound, which interprets it as a panning
//   value with a unit of 1/100 dB
//
// By transforming the calculation to pixel space and full decibels, we end up
// with ((16 / 64) × 100) = 25 pixels per shifted decibel.

// Ｘ座標の中心のデフォルト値
constexpr int SND_X_MID = PixelToWorld(320);

constexpr int SND_X_PER_DECIBEL = PixelToWorld(25);
// -----------------------------------------------------------
