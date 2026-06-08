/*
 *   Replay functions
 *
 */

#pragma once

#include "hatoyama/api/export.h"

#ifdef __cplusplus
import std.compat;
#endif

#ifdef __cplusplus
namespace Replay {
// Returns the game's original expected filename of the single one-stage replay
// for the given [stage] in Shuusou Gyoku's original format, or an empty view
// for invalid [stage] numbers.
HATOYAMA_API std::u8string_view OldBasenameFor(uint8_t stage);
} // namespace Replay
#endif
