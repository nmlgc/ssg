/*
 *   Public FFI functions
 *
 */

#pragma once

#include "hatoyama/api/export.h"

typedef struct C_SSG C_SSG;
typedef struct PACKFILE_READ PACKFILE_READ;

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

/// Packfiles
/// ---------
/// Corresponding to `internal/LZ.hpp`.

// Validates the header and checksums of the given packfile buffer, then
// returns a newly allocated pointer to a packfile instance. This instance
// contains a pointer to [buf], which must be valid during its entire lifetime
// of the instance. Returns `NULL` if [buf] is not a valid Shuusou Gyoku
// packfile, or if out of memory.
HATOYAMA_API PACKFILE_READ* PackfileNew(const void *buf, size_t size);

// Deallocates a packfile instance allocated by PackfileNew(). Always returns
// `NULL`.
HATOYAMA_API PACKFILE_READ* Packfile_Free(PACKFILE_READ *packfile);
/// -----------------
#ifdef __cplusplus
}
#endif
