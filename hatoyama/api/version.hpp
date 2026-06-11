/*
 *   Version info (of the game, not the engine)
 *
 */

#pragma once

#include "hatoyama/api/export.h"

import std.compat;

namespace Version {
// Returns the ReC98 build version. Increments with every release.
HATOYAMA_API std::u8string_view Build(void);

// Returns the declared game logic version. Supposed to stay constant without
// intentional changes to gameplay.
HATOYAMA_API std::u8string_view Logic(void);
}
