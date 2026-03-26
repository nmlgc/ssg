/*                                                                           */
/*   Maid.h   メイドさん関連の処理                                           */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_MAID_H
#define PBGWIN_MAID_H		"めいど : Version 0.01 : Update 2000/02/17"
//#pragma message(PBGWIN_MAID_H)

#include "ssg/Input.h"



///// [ 定数 ] /////

// サボテン定数 //
#define VIVDEAD_VAL		(300)		// びびっとの死亡時間．．．

#define MAID_MOVE_DISABLE_TIME		(250-100)		// 行動不能な時間

#define BOMBMUTEKI_VAL	60			// ボムの終端無敵時間
#define SBOPT_DX		26			// オプションのずれ幅(x64ではない)
#define EVADETIME_MAX	256			// かすりマックス時の待ち時間




///// [マクロ] /////



///// [構造体] /////

struct MAID {
	int x,y;			// 現在の<表示>座標

	int vx,vy;			// オプションのズレ具合
	int opx,opy;		// 現在のオプション基本座標

	int64_t	score; 	// 得点カウンタ
	int64_t	dscore;	// 得点増加値

	uint32_t	evade_sum;	// かすり合計
	int		evadesc;		// かすり得点
	uint16_t	evade;	// かすり回数
	uint16_t	evade_c;	// 連続「かすり」の残り許容時間

	char v;				// サボテンの移動速度基本値(後で64~45倍にする)

	uint8_t weapon;	// "とげ" の種類
	uint8_t exp;	// サボテンの経験値？
	uint8_t bomb;	// ボムの数
	uint8_t left;	// 残りサボテン数
	uint8_t credit;	// のこりクレジット

	uint8_t GrpID;	// 表示すべきグラフィック

	uint16_t bomb_time;	// ボムウェイト用
	uint16_t exp2;	// 経験値増加抑制用
	uint16_t muteki;	// 無敵フラグ(0:off !0:無敵時間カウンタ)
	uint16_t lay_time;	// レーザーの発射タイミング用
	uint8_t lay_grp;	// レーザーのグラフィック用
	uint8_t toge_time;	// "とげ" の発射タイミング用
	uint8_t toge_ex;	// とげ発射用特殊変数
	uint8_t ShiftCounter;	// 押しっぱなし低速移動用

	bool bGameOver;	// ゲームオーバー判定用フラグ
	bool BuzzSound;	// かすった音を連続再生させないためのフラグ
};



///// [ 関数 ] /////
extern void MaidDraw(void);
extern void StateDraw(void);		// 各種ステータスを描画する
extern void MaidMove(INPUT_BITS, bool in_msg);
extern void MaidSet(void);			// 初期化
extern void MaidNextStage(void);	// 次のステージの準備
extern void MaidDead(void);			// 死す

extern void evade_add(uint8_t n);	// かすりゲージを上昇させる
extern void evade_addEx(int x, int y, uint8_t n);	// かすりゲージを上昇させる
extern void score_add(int sc);		// スコアを上昇させる
void PowerUp(uint8_t damage);

extern void WideBombDraw(void);		// ワイドショット用のボム(やや例外処理)



///// [ 変数 ] /////
extern MAID			Viv;					// 麗しきメイドさん構造体



uint8_t GetLaserDeg(void);
uint8_t GetRightLaserDeg(uint8_t LaserDeg, int i);
uint8_t GetLeftLaserDeg(uint8_t LaserDeg, int i);


#endif
