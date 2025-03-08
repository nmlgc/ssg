/*
 *   Cross-platform input declarations
 *
 */

#pragma once

import std.compat;

// Current pressed/released state for all virtual KEY_* keys.
using INPUT_BITS = uint16_t;

// 0 = unmapped.
using INPUT_PAD_BUTTON = uint8_t;

// キーボード定数 //
// Braced initializers cause a compile error if the constants don't fit within
// the INPUT_BITS type.
constexpr INPUT_BITS KEY_UP     = { 0x0001 };
constexpr INPUT_BITS KEY_DOWN   = { 0x0002 };
constexpr INPUT_BITS KEY_LEFT   = { 0x0004 };
constexpr INPUT_BITS KEY_RIGHT  = { 0x0008 };
constexpr INPUT_BITS KEY_TAMA   = { 0x0010 };
constexpr INPUT_BITS KEY_BOMB   = { 0x0020 };
constexpr INPUT_BITS KEY_SHIFT  = { 0x0040 };
constexpr INPUT_BITS KEY_RETURN = { 0x0080 };
constexpr INPUT_BITS KEY_ESC    = { 0x0100 };

constexpr INPUT_BITS KEY_ULEFT  = (KEY_UP | KEY_LEFT);
constexpr INPUT_BITS KEY_URIGHT = (KEY_UP | KEY_RIGHT);
constexpr INPUT_BITS KEY_DLEFT  = (KEY_DOWN | KEY_LEFT);
constexpr INPUT_BITS KEY_DRIGHT = (KEY_DOWN | KEY_RIGHT);

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
