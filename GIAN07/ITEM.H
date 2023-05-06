/*                                                                           */
/*   Item.h   アイテムの処理                                                 */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_ITEM_H
#define PBGWIN_ITEM_H		"ITEM : Version 0.01 : Update 2000/03/11"
//#pragma message(PBGWIN_ITEM_H)

#include "GIAN.H"
#include <array>


///// [ 定数 ] /////

// 最大数 //
#define ITEM_MAX	100

// 種類もしくは状態 //
#define ITEM_DELETE		0x00		// 消去要請
#define ITEM_SCORE		0x01		// 得点アイテム
#define ITEM_EXTEND		0x02		// 残りメイド数ＵＰ
#define ITEM_BOMB		0x03		// ボム

// その他 //
#define ITEM_GRAVITY	3				// アイテムに対するｙ加速度
#define ITEM_HITX		(( 8+8)*64)		// アイテムのＸ当たり判定
#define ITEM_HITY		((16+8)*64)		// アイテムのＹ当たり判定


///// [構造体] /////
typedef struct tagITEM_DATA{
	int		x,y;
	int		vx,vy;
	DWORD	count;
	BYTE	type;
} ITEM_DATA;



///// [ 関数 ] /////
void ItemSet(int x, int y, uint8_t type);	// アイテムを発生させる
void ItemMove(void);	// アイテムを動かす
void ItemDraw(void);	// アイテムを描画する

void ItemIndSet(void);	// アイテム配列の初期化



///// [ 変数 ] /////
extern std::array<ITEM_DATA, ITEM_MAX>	Item;
extern std::array<uint16_t, ITEM_MAX>	ItemInd;
extern WORD			ItemNow;



#endif
