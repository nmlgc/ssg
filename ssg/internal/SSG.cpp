/*
 *   Central controller
 *
 */

#include "SSG.hpp"
#include "LogicInstance.h"

void C_SSG::RoundInit(const ROUND_PARAMS& round, uint8_t stage_first)
{
	Round = round;
	MaidSet(stage_first);
	PlayRankReset();

	// 乱数の初期化 //
	// 最後に乱数もそろえる //
	RNG.seed = round.Seed;
}
