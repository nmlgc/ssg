/*
 *   Public API
 *
 */

#pragma once

#include "hatoyama/api/export.h"

#ifdef __cplusplus
import std.compat;
#else
typedef char char8_t;
#endif

extern "C" {
/// Version info
/// ------------
/// Corresponding to `version.hpp`.

// Returns the ReC98 build version. Increments with every release.
HATOYAMA_API const char8_t* VersionBuild(void);

// Returns the declared game logic version. Supposed to stay constant without
// intentional changes to gameplay.
HATOYAMA_API const char8_t* VersionLogic(void);
/// ------------
}
