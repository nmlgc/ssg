/*
 *   Format-independent background music interface
 *
 */

#pragma once

#include "game/hash.h"
#include "game/narrow.h"
#include "platform/buffer.h"
#include <chrono>
#include <stdint.h>

// Loads the BGM with the given 0-based [id] from the game's original BGM data
// source.
extern bool (*const BGM_MidLoadOriginal)(unsigned int id);

// Loads MIDI BGM from the given byte buffer.
extern bool (*const BGM_MidLoadBuffer)(BYTE_BUFFER_OWNED);

// Loads the source MIDI via its hash from the game's original BGM data source.
extern bool (*const BGM_MidLoadByHash)(const HASH& hash);

bool BGM_Init(void);
void BGM_Cleanup(void);

// General queries
// ---------------

bool BGM_Enabled(void);
bool BGM_HasGainFactor(void);
bool BGM_GainApply(void);
std::chrono::duration<int32_t, std::milli> BGM_PlayTime(void);
Narrow::string_view BGM_Title(void);
// ---------------

bool BGM_ChangeMIDIDevice(int8_t direction); // 出力デバイスを変更する

// Playback
// --------

// Stops the currently playing BGM, then loads and plays the track with the
// given 0-based [id]. Returns `true` if the BGM was changed successfully.
bool BGM_Switch(unsigned int id);

void BGM_Play(void);
void BGM_Stop(void);

void BGM_Pause(void);
void BGM_Resume(void);
// --------

// Volume control
// --------------

// Changes the gain apply flag, and applies the result to any currently playing
// waveform track.
void BGM_SetGainApply(bool apply);

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

// BGM pack management
// -------------------

// Returns whether at least one BGM pack exists under the BGM root directory.
// The result is cached and invalidated whenever BGM is paused.
bool BGM_PacksAvailable(bool invalidate_cache = false);

size_t BGM_PackCount(void);
void BGM_PackForeach(void func(const std::u8string&& str));

// Restarts any currently playing BGM when switching to a different [pack].
// Returns `false` if the given [pack] doesn't exist, and switches to the empty
// pack in that case.
bool BGM_PackSet(const std::u8string_view pack);
// -------------------
