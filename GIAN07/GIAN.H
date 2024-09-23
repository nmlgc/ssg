/*                                                                           */
/*   GIAN.h   ゲーム全体の管理                                               */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_GIAN_H
#define PBGWIN_GIAN_H		"GIAN : Version 0.62 : Update 2000/02/23"
//#pragma message(PBGWIN_GIAN_H)



///// [更新履歴] /////

// 2000/02/23 : GIAN06 同等の性能をもつようになった。
// 2000/02/09 : 大幅な変更



//#define PBG_DEBUG		// デバッグモードを可能にする時に定義する



///// [Include Files] /////
#include "constants.h"

#include "BOSS.H"				// ボスの定義＆ボス用エフェクトなど
#include "PRankCtrl.h"			// プレイランク管理
#include "GAMEMAIN.H"			// メインのルーチン切り替え処理
#include "EFFECT.H"				// 主にテキストベースのエフェクト処理
#include "EFFECT3D.H"			// ３Ｄエフェクト
#include "FRAGMENT.H"			// 破片系エフェクト処理
#include "LASER.H"				// 短いレーザー＆リフレクトレーザー処理
#include "LLASER.H"				// なが～いレーザーの処理
#include "HOMINGL.H"			// ホーミングレーザーの処理
#include "MAIDTAMA.H"			// 自機ショットの処理
#include "MAID.H"				// その名の通り
#include "SCROLL.H"				// 背景スクロール＆ＳＣＬ管理
#include "LOADER.H"				// 各種ローダー

#include "ITEM.H"				// アイテム処理


///// [ 定数 ] /////

// 座標関連 //
#define X_MIN		(128)						// 表示Ｘ座標最小値
#define X_MAX		(511)						// 表示Ｘ座標最大値
#define X_MID		((X_MAX+X_MIN)/2)
#define Y_MIN		(0)							// 表示Ｙ座標最小値
constexpr WINDOW_COORD Y_MAX = (GRP_RES.h - 1);
#define Y_MID		((Y_MAX+Y_MIN)/2)

constexpr WINDOW_LTRB PLAYFIELD_CLIP = {
	X_MIN, Y_MIN, (X_MAX + 1), (Y_MAX + 1)
};

#define X_RNDV		(-30000)				// Ｘ座標のランダム指定用
#define Y_RNDV		(-30000)				// Ｙ座標のランダム指定用

#define GX_MIN		(X_MIN * 64)			// ゲーム座標上におけるＸ座標最小値
#define GX_MAX		(X_MAX * 64)			// ゲーム座標上におけるＸ座標最大値
#define GX_MID		((GX_MAX+GX_MIN)/2)		// ゲーム座標上におけるＸ座標中心値
#define GY_MIN		(Y_MIN * 64)			// ゲーム座標上におけるＹ座標最小値
#define GY_MAX		(Y_MAX * 64)			// ゲーム座標上におけるＹ座標最大値
#define GY_MID		((GY_MAX+GY_MIN)/2)		// ゲーム座標上におけるＹ座標中心値

#define SX_WID		(64*10)						// サボテンのＸ幅？
#define SY_WID		(64*10)						// サボテンのＹ幅？
#define SX_MIN		(GX_MIN + SX_WID)			// サボテンのＸ座標最小値
#define SX_MAX		(GX_MAX - SX_WID)			// サボテンのＸ座標最大値
#define SY_MIN		(GY_MIN + SY_WID + 30*64)	// サボテンのＹ座標最小値
#define SY_MAX		(GY_MAX - SY_WID)			// サボテンのＹ座標最大値
#define SX_START	(GX_MID)					// サボテンの開始Ｘ座標
#define SY_START	(GY_MAX+180*64 /*- 50*64*/)			// サボテンの開始Ｙ座標

#define RL_WIDX		(32-4)					// 反射レーザー用_Ｘ座標_補正値
#define RL_WIDY		16						// 反射レーザー用_Ｙ座標_補正値
#define RLX_MIN		(GX_MIN/64+RL_WIDX)		// 反射レーザーの反射Ｘ座標最小値
#define RLX_MAX		(GX_MAX/64-RL_WIDX)		// 反射レーザーの反射Ｘ座標最大値
#define RLY_MIN		(GY_MIN/64+RL_WIDY)		// 反射レーザーの反射Ｙ座標最小値
#define RLY_MAX		(GX_MAX/64-RL_WIDY)		// 反射レーザーの反射Ｙ座標最大値

#define NREG_SX		(X_MID-13*9)			// ネームレジスト用ウィンドウの開始Ｘ
#define NREG_SY		(Y_MID+100)				// ネームレジスト用ウィンドウの開始Ｙ
#define NREGI_X		(X_MID-8*7)				// ネームレジスト用ウィンドウ(名前表示部)の開始Ｘ
#define NREGI_Y		(Y_MID+60)				// ネームレジスト用ウィンドウ(名前表示部)の開始Ｙ

#define STG_RNDXY	0				// 配置する座標がランダムの時の値...(なんだかよう分からん)


// スコア //
#define SCORE_NAME		9			// スコア用の最大文字列長(NULL含む)


///// [マクロ] /////
#define SPEEDM(v)	((short)(((v)&0x3f)<<4))	// 速度セット用まくろ(BYTE->int?)
#define WAVESP(v)	((short)((v)<<4))			// WAVE?用の速度セット(まずいな)

#define GX_RND	((X_MIN+rnd()%(X_MAX-X_MIN))<<6)			// Ｘ座標ランダム
#define GY_RND	((Y_MIN+rnd()%(Y_MAX-Y_MIN))<<6)			// Ｙ座標ランダム

//#define HITCHK(a,b,h)	( ((uint32_t)(a)-(b)+(h))<h*2 )	// ヒットなら非ゼロ
#define HITCHK(a,b,h)	(abs((a)-(b))<(h))


///// [構造体] /////

/*
// スコア管理用構造体 //
typedef struct tagSCORE_DATA{
	__int64	score;
	uint8_t	weapon;
	char	name[SCORE_NAME];
} SCORE_DATA;

typedef struct tagHIGH_SCORE{
	SCORE_DATA		easy[8];
	SCORE_DATA		normal[8];
	SCORE_DATA		hard[8];
	SCORE_DATA		lunatic[8];
} HIGH_SCORE;
*/


///// [グローバル変数] /////
//extern HIGH_SCORE	*HighScore;
//extern char			ScoreTable[8][80];
extern uint32_t	GameCount;
extern uint8_t	GameStage;
extern uint8_t	GameLevel;


extern void StdStatusOutput(void);



#endif
