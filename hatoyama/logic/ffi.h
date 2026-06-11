/*
 *   Elementary declarations for FFI headers
 *
 */

#pragma once

#ifdef __cplusplus
import std.compat;
#else
#include <stddef.h>
#include <stdint.h>

// UTF-8 character.
typedef char char8_t;
#endif

// Booleans for FFI types. 0 is `false`, non-0 is `true`.
typedef uint8_t bool8_t;
