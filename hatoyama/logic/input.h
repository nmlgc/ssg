/*
 *   Cross-platform input declarations and state
 *
 */

#pragma once

#include "ssg/Input.h"

// 0 = unmapped.
using INPUT_PAD_BUTTON = uint8_t;

// Returns whether this key represents an "OK" action.
bool Input_IsOK(INPUT_BITS key);

// Returns whether this key represents a "Cancel" action.
bool Input_IsCancel(INPUT_BITS key);

// Returns the delta that this key would apply to a numeric option value.
int_fast8_t Input_OptionKeyDelta(INPUT_BITS key);

// Additional virtual keys for inputs that were read using GetAsyncKeyState()
// in the original game. Treated separately to not complicate any existing
// comparisons of [Key_Data] with 0.
using INPUT_SYSTEM_BITS = uint16_t;

constexpr INPUT_SYSTEM_BITS SYSKEY_SNAPSHOT       = { 0x0001 };
constexpr INPUT_SYSTEM_BITS SYSKEY_SKIP           = { 0x0002 };
constexpr INPUT_SYSTEM_BITS SYSKEY_BGM_FADE       = { 0x0004 };
constexpr INPUT_SYSTEM_BITS SYSKEY_BGM_DEVICE     = { 0x0008 };
constexpr INPUT_SYSTEM_BITS SYSKEY_GRP_FULLSCREEN = { 0x0010 };
constexpr INPUT_SYSTEM_BITS SYSKEY_GRP_SCALE_DOWN = { 0x0020 };
constexpr INPUT_SYSTEM_BITS SYSKEY_GRP_SCALE_UP   = { 0x0040 };
constexpr INPUT_SYSTEM_BITS SYSKEY_GRP_SCALE_MODE = { 0x0080 };
constexpr INPUT_SYSTEM_BITS SYSKEY_GRP_TURBO      = { 0x0100 };
constexpr INPUT_SYSTEM_BITS SYSKEY_GRP_API        = { 0x0200 };

using INPUT_PAD_BINDING = std::pair<const INPUT_PAD_BUTTON&, INPUT_BITS>;

// グローバル変数(Public) //
extern INPUT_BITS Key_Data;
extern INPUT_BITS Pad_Data;
extern INPUT_SYSTEM_BITS SystemKey_Data;

// Initialized by game code.
extern std::span<const INPUT_PAD_BINDING> Key_PadBindings;
