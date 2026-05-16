/*
 *   The single global game logic instance
 *
 */

#include "LogicInstance.h"
#include "ssg/internal/RNG.hpp"

ROUND_PARAMS Round;
C_RNG RNG; // Temporary...

// Just default-constructing the logic structure at global scope...
C_SSG SSG;

// …avoids a reliance on named return value optimization here.
int SSG_Init = ([] {
	return 0;
})();
