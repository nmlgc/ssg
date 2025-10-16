/*                                                                           */
/*   Score.h   スコア入出力関数                                              */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_SCORE_H
#define PBGWIN_SCORE_H		"SCORE : Version 0.01 : Update 2000/07/29"
//#pragma message(PBGWIN_SCORE_H)

import std.compat;
#include "game/string_format.h"



///// [ 定数 ] /////
#define NR_NAME_LEN		9		// ネームレジストの名前の長さ('\0' 含む)
#define NR_RANK_MAX		5		// 順位付け(Save)される最大数



///// [構造体] /////

typedef struct tagNR_NAME_DATA{
	char		Name[NR_NAME_LEN];		// 名前
	int64_t	Score;	// スコア
	uint32_t	Evade;	// かすり回数
	uint8_t	Stage;	// ステージ
	uint8_t	Weapon;	// 装備品
} NR_NAME_DATA;

typedef struct tagNR_SCORE_DATA{
	NR_NAME_DATA	Easy[NR_RANK_MAX];		// 難易度：Ｅａｓｙ
	NR_NAME_DATA	Normal[NR_RANK_MAX];	// 難易度：Ｎｏｒｍａｌ
	NR_NAME_DATA	Hard[NR_RANK_MAX];		// 難易度：Ｈａｒｄ
	NR_NAME_DATA	Lunatic[NR_RANK_MAX];	// 難易度：Ｌｕｎａｔｉｃ
	NR_NAME_DATA	Extra[NR_RANK_MAX];		// 難易度：Ｅｘｔｒａ
} NR_SCORE_DATA;

typedef struct tagNR_SCORE_STRING{
	uint8_t	Rank;	// 実際の順位(ある順位が複数ある場合の対策)
	int		x,y;					// 描画用座標
	bool	bMoveEnable;	// 移動可能か？

	char	Name[NR_NAME_LEN];		// 名前
	char	Score[STRING_NUM_CAP<decltype(NR_NAME_DATA::Score)> + 1]; // 得点
	char	Evade[STRING_NUM_CAP<decltype(NR_NAME_DATA::Evade)> + 1]; // かすり
	char	Stage[STRING_NUM_CAP<decltype(NR_NAME_DATA::Stage)> + 1]; // ステージ
	uint8_t	Weapon;	// 装備
} NR_SCORE_STRING;



///// [ 関数 ] /////

// 現在のスコア列を取得する(Ret:下と同じ)
uint8_t SetScoreString(NR_NAME_DATA *NData, uint8_t Dif);

// ０：ハイスコアでない  それ以外：順位
uint8_t IsHighScore(const NR_NAME_DATA *NData, uint8_t Dif);

bool SaveScoreData(NR_NAME_DATA *NData, uint8_t Dif);	// スコアデータを書き出す



///// [グローバル変数] /////
extern NR_SCORE_STRING ScoreString[NR_RANK_MAX];	// スコアデータ文字列格納先


#endif
