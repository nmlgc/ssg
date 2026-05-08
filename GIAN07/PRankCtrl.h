/*                                                                           */
/*   PRankCtrl.h   プレイランク管理                                          */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_PRANKCTRL_H
#define PBGWIN_PRANKCTRL_H		"PRANKCTRL : Version 0.01 : Update 2000/09/13"

import std.compat;



///// [構造体] /////
struct PlayRankInfo {
	uint8_t	LevelRanked;	// 方向数も関係する難易度変化
	int		Rank;			// 弾の速度変化に関する値
};

class C_PLAYRANK {
private:
	PlayRankInfo PlayRank;

public:
	// 難易度の許容範囲内でプレイランクを増減する
	void Add(int n);

	// 現在の難易度に応じてプレイランクを初期化
	void Reset(void);

	const auto LevelRanked(void) const {
		return PlayRank.LevelRanked;
	}
	const auto Rank(void) const {
		return PlayRank.Rank;
	}
};



///// [グローバル変数] /////
extern C_PLAYRANK PlayRank;



#endif
