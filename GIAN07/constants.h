/*
 *   Game-specific compile-time constants and types to be shared with the
 *   platform layers
 *
 */

#pragma once

import std.compat;
#include "game/coords.h"

// Yup, the game is supposed to be running at 62.5 FPS.
constexpr auto FRAME_TIME_TARGET = 16;

enum class FONT_ID : uint8_t {
	SMALL = 0,	// フォント(小さい文字用)
	NORMAL = 1,	// フォント(通常の文字用)
	LARGE = 2,	// フォント(大きい文字用)
	COUNT,
};

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
