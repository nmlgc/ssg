/*
 *   Format-independent background music interface
 *
 */

#pragma once

#include "game/narrow.h"
#include "platform/buffer.h"
#include <chrono>
#include <stdint.h>

// Loads the BGM with the given 0-based [id] from the game's original BGM data
// source.
extern bool (*const BGM_MidLoadOriginal)(unsigned int id);

// Loads MIDI BGM from the given byte buffer.
extern bool (*const BGM_MidLoadBuffer)(BYTE_BUFFER_OWNED);

bool BGM_Init(void);
void BGM_Cleanup(void);

// General queries
// ---------------

bool BGM_Enabled(void);
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

// Restarts any currently playing BGM when switching to a different [pack].
void BGM_PackSet(const std::u8string_view pack);
// -------------------
