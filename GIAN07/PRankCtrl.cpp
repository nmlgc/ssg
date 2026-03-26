/*                                                                           */
/*   PRankCtrl.cpp   プレイランク管理                                        */
/*                                                                           */
/*                                                                           */

#include "PRankCtrl.h"
#include "GIAN.H"
#include "LEVEL.H"

PlayRankInfo	PlayRank;

struct RANK_DATA_FOR_DIFFICULTY {
	int clamp_min;
	int threshold;
	int initial;
	int clamp_max;
};

// イージー 　　　0 ～ 24
// ノーマル　　　16 ～ 40
// ハード　　 　　32 ～ 48
// ルナティック  40 ～ 64
static const RANK_DATA_FOR_DIFFICULTY RANK_DATA[] = {
	// clamp_min   threshold,    initial   clamp_max
	{ ( 0 * 256), ( 0 * 256), (12 * 256), (24 * 256) }, // Easy
	{ (16 * 256), (20 * 256), (28 * 256), (40 * 256) }, // Normal
	{ (32 * 256), (36 * 256), (40 * 256), (48 * 256) }, // Hard
	{ (40 * 256), (44 * 256), (52 * 256), (64 * 256) }, // Lunatic
};



// 難易度の許容範囲内でプレイランクを増減する
void PlayRankAdd(int n)
{
	// 難易度を変化させる //
	if(GameStage == STAGE_EXTRA) {
		if(n > 0) {
			PlayRank.Rank += (std::max)(+1, (n /  4));
		} else if(n < 0) {
			PlayRank.Rank += (std::min)(-1, (n / 10));
		}
	}
	else{
		PlayRank.Rank += n;
	}

	// この分岐に関しては、基本的にコンフィグの値に基づく //
	assert(
		(LevelSelected <= GAME_LUNATIC) && "Extra is not a valid difficulty"
	);
	const auto& rd_selected = RANK_DATA[LevelSelected];
	PlayRank.Rank = std::clamp(
		PlayRank.Rank, rd_selected.clamp_min, rd_selected.clamp_max
	);

	// We can only jump up or down by a single difficulty.
	if((LevelSelected < GAME_LUNATIC) &&
	   (PlayRank.Rank >= RANK_DATA[LevelSelected + 1].threshold)) {
		PlayRank.LevelRanked = (LevelSelected + 1);
	} else if(PlayRank.Rank >= rd_selected.threshold) {
		PlayRank.LevelRanked = LevelSelected;
	} else if(LevelSelected > GAME_EASY) {
		PlayRank.LevelRanked = (LevelSelected - 1);
	}
}


// 現在の難易度に応じてプレイランクを初期化
void PlayRankReset(void)
{
	assert(
		(LevelSelected <= GAME_LUNATIC) && "Extra is not a valid difficulty"
	);
	PlayRank.LevelRanked = LevelSelected;
	PlayRank.Rank = RANK_DATA[LevelSelected].initial;
}
