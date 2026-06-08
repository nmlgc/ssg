/*
 *   Public FFI functions
 *
 */

#pragma once

#include "hatoyama/api/export.h"

typedef struct C_SSG C_SSG;

#ifdef __cplusplus
extern "C" {
#endif
/// Gameplay instance
/// -----------------
/// Corresponding to `internal/SSG.hpp`.

// Allocates a new gameplay instance. The returned pointer has to be freed via
// SSG_Free(). Returns `NULL` if out of memory.
HATOYAMA_API C_SSG* SSGNew(void);

// Deallocates a gameplay instance allocated by SSGNew(), along with any data.
// Always returns NULL.
HATOYAMA_API C_SSG* SSG_Free(C_SSG *ssg);
/// -----------------
#ifdef __cplusplus
}
#endif
