/*
 *   FFI helper types
 *
 */

#pragma

#ifdef __cplusplus
import std.compat;
#else
#include <stdint.h>
#endif

// Booleans for FFI types. 0 is `false`, non-0 is `true`.
typedef uint8_t bool8_t;
