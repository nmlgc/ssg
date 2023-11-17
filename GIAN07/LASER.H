/*************************************************************************************************/
/*   LASER.H   レーザーに関する処理(反射,ショート)                                               */
/*                                                                                               */
/*************************************************************************************************/

#ifndef PBGWIN_LASER_H
#define PBGWIN_LASER_H		"LASEER : Ver 0.51 : Update 2000/02/17"
//#pragma message(PBGWIN_LASER_H)

#include "game/coords.h"

///// [更新履歴] /////

// 2000/02/17 : 新しいシステムに移行開始。無限遠レーザーと完全に分離

/*-> ここからはちょっと古いよ(1999..)
 * (4/3)  10:36 開発開始
 * (4/6)  12:00 ついにポリゴン＆クリッピング関数が完成。描画はいつ出来るのか？
 * (4/7)  12:02 全てのレーザーを同じ構造体で扱う事にした
 * (4/8)   7:23 無限遠レーザーの制作
 * (4/9)   2:01 反射レーザーを一応打ち込み終わる
 * (4/9)   2:59 反射レーザー完成
 * (4/11) 14:05 ショート＆反射レーザーの当たり判定完成
 * (4/11) 15:17 リフレクターのヒットチェックを強化(バグは消えたが遅くなった)
 *
 * (9/23) 16:18 ライン描画、ＥＣＬ対応などが完了
 */


////レーザー定数////
#define LASER_MAX			1000			// レーザーの最大発生本数


////レーザー発動コマンド構造体////
typedef struct{
	int  x,y;		// 始点の座標
	int  v;			// レーザーの初速度

	int w;			// レーザーの太さ        (x64座標を使用する)
	int l;			// レーザーの長さ最終値  (x64座標を使用する)
	int l2;			// レーザーの発射位置補正(x64...)

	uint8_t d;	// 発射角
	uint8_t dw;	// 発射幅

	uint8_t n;	// レーザーの本数
	uint8_t c;	// レーザーの色

	char a;			// 加速度(つかうのかな???)
	uint8_t cmd;	// レーザー発動コマンド(ほとんど弾と同じかも)
	uint8_t type;	// ショート、無限遠など
	uint8_t notr;	// 反射しないリフレクターの番号
} LASER_CMD;


/*
////反射物(鏡?) 構造体////
typedef struct{
	int		x,y;		// 反射物の中心座標

	uint32_t	l;	// リフレクターの長さ(中心から先端まで,つまり全体でl*2)
	uint8_t	d;	// 反射物の角度(0 <= d < 128)
} REFLECTOR;
*/

////レーザー関数////
extern void laser_set(void);		// レーザーをセットする(難易度変更"有り")
extern void laser_setEX(void);		// レーザーをセットする(難易度変更"無し")
extern int  llaser_set(uint16_t *ind);	// 無限遠レーザーをセットする(セットできた数を返すのだ)
extern void laser_move(void);		// レーザーを動かす
extern void laser_draw(void);		// レーザーを描画する
extern void laser_clear(void);		// レーザー全てに消去エフェクトをかける
extern void laserind_set(void);		// レーザー順序用配列の初期化


////レーザーの各種変数たち////
extern LASER_CMD	LaserCmd;				// 標準レーザーコマンド構造体
extern uint16_t	LaserNow;	// レーザーの本数
//extern REFLECTOR	Reflector[RT_MAX];		// 反射物構造体
//extern uint16_t	ReflectorNow;	// 反射物の個数

#endif
