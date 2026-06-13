/*
 *   Public FFI functions
 *
 */

#pragma once

#include "ssg/Replay.h"

typedef struct C_SSG C_SSG;
typedef struct PACKFILE_READ PACKFILE_READ;
typedef struct REPLAY_OLD REPLAY_OLD;

#ifdef __cplusplus
import std.compat;

namespace Logic {
// Returns the name of the packfile containing all game logic data.
HATOYAMA_API const std::u8string_view PackfileBasename(void);
} // namespace Logic

extern "C" {
#else
#include <stdint.h>
typedef char char8_t;
#endif

// Returns the name of the packfile containing all game logic data.
HATOYAMA_API const char8_t *LogicPackfileBasename(void);

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

/// Replays
/// -------
/// Corresponding to `Replay.h`.

// Returns the game's original expected filename of the single one-stage replay
// for the given [stage] in Shuusou Gyoku's original format in UTF-8, or `NULL`
// for invalid [stage] numbers.
HATOYAMA_API const char8_t* ReplayOldBasenameFor(uint8_t stage);

// Detects which stage the given one-stage UTF-8 replay filename corresponds
// to, by comparing the filename portion of [fn] against several encodings of
// the original Japanese replay file name prefix. Returns 0 if detection
// failed.
HATOYAMA_API uint8_t ReplayOldStageNumDetect(const char8_t *fn);

// Loads a one-stage replay from a compressed buffer. Returns `NULL` if
// [packfile_buf] is not a valid old-format replay, or if out of memory.
HATOYAMA_API REPLAY_OLD* ReplayOldLoad(
	const void *packfile_buf, size_t packfile_size
);

// Returns non-owning pointers to the data contained in `REPLAY_OLD`. These
// pointers may be null if [replay] is `NULL` or invalid.
HATOYAMA_API REPLAY_OLD_PTRS ReplayOld_Data(const REPLAY_OLD *replay);

HATOYAMA_API REPLAY_OLD *ReplayOld_Free(REPLAY_OLD *replay);
/// -------
#ifdef __cplusplus
}
#endif
