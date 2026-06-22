/*
 *   Elementary declarations for FFI headers
 *
 */

#pragma once

#ifdef __cplusplus
import std.compat;
#else
#include <stdint.h>
#endif

// Booleans for FFI types. 0 is `false`, non-0 is `true`.
typedef uint8_t bool8_t;
