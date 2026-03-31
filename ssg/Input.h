/*
 *   Input bits
 *
 */

#pragma once

#ifdef __cplusplus
import std.compat;
#else
#include <stdint.h>
#endif

// Current pressed/released state for all virtual KEY_* keys.
typedef uint16_t INPUT_BITS;

// We obviously can't use `constexpr` in FFI headers.
#pragma warning(push)
#pragma warning(disable: 26814)

// キーボード定数 //
static const INPUT_BITS KEY_UP     = { 0x0001 };
static const INPUT_BITS KEY_DOWN   = { 0x0002 };
static const INPUT_BITS KEY_LEFT   = { 0x0004 };
static const INPUT_BITS KEY_RIGHT  = { 0x0008 };
static const INPUT_BITS KEY_TAMA   = { 0x0010 };
static const INPUT_BITS KEY_BOMB   = { 0x0020 };
static const INPUT_BITS KEY_SHIFT  = { 0x0040 };
static const INPUT_BITS KEY_RETURN = { 0x0080 };
static const INPUT_BITS KEY_ESC    = { 0x0100 };
static const INPUT_BITS KEY_SKIP   = { 0x0200 };

#pragma warning(pop)
