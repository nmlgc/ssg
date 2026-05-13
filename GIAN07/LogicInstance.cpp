/*
 *   The single global game logic instance
 *
 */

#include "LogicInstance.h"
#include "BossHPG.h"
#include "ECL.H"
#include "ECLFront.h"
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
	SSG.Hooks.ECL_Op = ECL_Frontend,
	SSG.Hooks.Snd_SEPlay = [](uint8_t id, int x, bool8_t loop, HOOKS *) {
		Snd_SEPlay(id, x, loop);
	};
	SSG.Hooks.Snd_SEStop = [](uint8_t id, HOOKS *) {
		Snd_SEStop(id);
	};
	SSG.Hooks.Boss_HPSumAtStart = [](const uint32_t hp_sum, HOOKS *) {
		BossHPG_Move(hp_sum);
	};
	SSG.Hooks.Boss_Defeat = [](const BOSS_DATA *, HOOKS *) {
		ScrollCommand(SCMD_QUAKE);
	};
#ifdef SCRIPT_TRACE
	std::ranges::fill(SSG.Hooks.ecl_hook_flag, 1);
#else
	// These three trigger frontend-exclusive graphical effects and must
	// always be active.
	SSG.Hooks.ecl_hook_flag[ECL_BOSSSET] = true;
	SSG.Hooks.ecl_hook_flag[ECL_CEFC] = true;
	SSG.Hooks.ecl_hook_flag[ECL_STG3EFC] = true;
#endif
	return 0;
})();
