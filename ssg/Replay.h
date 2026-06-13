/*
 *   Replay functions
 *
 */

#pragma once

#include "hatoyama/api/export.h"
#include "hatoyama/logic/ffi.h"

#ifdef __cplusplus
namespace Replay {
// Returns the game's original expected filename of the single one-stage replay
// for the given [stage] in Shuusou Gyoku's original format, or an empty view
// for invalid [stage] numbers.
HATOYAMA_API std::u8string_view OldBasenameFor(uint8_t stage);

// Detects the stage that the given one-stage replay filename corresponds to,
// by comparing the filename portion of [fn] against several encodings of the
// original Japanese replay filename prefix. Returns 0 if detection failed.
HATOYAMA_API uint8_t OldStageNumDetect(std::u8string_view fn);
} // namespace Replay
#endif
