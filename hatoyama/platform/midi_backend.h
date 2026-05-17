/*
 *   Platform-specific MIDI backend interface
 *
 */

#pragma once

#ifdef WIN32
#define SUPPORT_MIDI_BACKEND
#endif

#ifdef SUPPORT_MIDI_BACKEND

import std.compat;
#include "game/narrow.h"

// Initializes the backend with a default output device.
bool MidBackend_Init(void); // ＭＩＤＩ関連初期化

// Shuts down the backend.
void MidBackend_Cleanup(void); // ＭＩＤＩ関連おしまい

// Returns the name of the current MIDI device, or std::nullopt if the backend
// is not initialized. Can also be used for general initialization checks.
std::optional<Narrow::string_view> MidBackend_DeviceName(void);

// Switches to the next working output device in the given positive or negative
// [direction].
bool MidBackend_DeviceChange(int8_t direction); // 出力デバイスを変更する

// Starts a timer that periodically calls Mid_Proc().
void MidBackend_StartTimer(void);

// Stops the timer.
void MidBackend_StopTimer(void);

// Sends a raw MIDI event to the active device. [event] unfortunately has to be
// mutable because MIDIHDR on Windows wants a non-const pointer...
void MidBackend_Out(uint8_t byte_1, uint8_t byte_2, uint8_t byte_3 = 0x00);
void MidBackend_Out(std::span<uint8_t> event);

// Stops all currently playing notes.
void MidBackend_Panic(void);

#endif
