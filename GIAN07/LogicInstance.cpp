/*
 *   The single global game logic instance
 *
 */

#include "LogicInstance.h"
#include "SCROLL.H"
#include "ssg/Hook.h"
#include "ssg/internal/RNG.hpp"
#include "hatoyama/engine/snd.h"

ROUND_PARAMS Round;
C_RNG RNG; // Temporary...

// Just default-constructing the logic structure at global scope...
C_SSG SSG;

// …avoids a reliance on named return value optimization here.
int SSG_Init = ([] {
	SSG.Hooks.SCL_Op = SCL_Frontend,
	SSG.Hooks.Snd_SEPlay = [](uint8_t id, int x, bool8_t loop, HOOKS *) {
		Snd_SEPlay(id, x, loop);
	};
	SSG.Hooks.Snd_SEStop = [](uint8_t id, HOOKS *) {
		Snd_SEStop(id);
	};
	return 0;
})();
