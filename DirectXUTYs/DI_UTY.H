/*                                                                           */
/*   DI_UTY.h   Direct Input 関連の関数                                      */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_DI_UTY_H
#define PBGWIN_DI_UTY_H		"DI_UTY : Ver 0.10"


// 更新履歴 //

// 2000/01/11 : .c->.cpp に変更、英語->日本語に変更。


#include "DX_ERROR.H"
#include "game/input.h"


// キーボード定義用構造体 //
typedef struct tagInputConfig{
	BYTE	KeyUp;			// 上方向に移動
	BYTE	KeyDown;		// 下方向に移動
	BYTE	KeyLeft;		// 左方向に移動
	BYTE	KeyRight;		// 右方向に移動
	BYTE	KeyTama;		// 弾発射＆決定
	BYTE	KeyBomb;		// ボム発動＆キャンセル
	BYTE	KeyShift;		// 低速移動

	BYTE	KeyULeft;			// 左上
	BYTE	KeyURight;			// 右上
	BYTE	KeyDLeft;			// 左下
	BYTE	KeyDRight;			// 右下

	DWORD	PadMask_Tama;		// パッド用マスク
	DWORD	PadMask_Bomb;		//
	DWORD	PadMask_Shift;		//
	DWORD	PadMask_Cancel;		//

	BOOL	bIsUsed[256];		// 多重定義防止用
} InputConfig;


// 関数プロトタイプ宣言 //
extern bool Key_Start(void);	// キー入力開始
extern void Key_Config(INPUT_BITS Key);	// あるキーボード定数に対するコンフィグ
extern void Key_End(void);							// キー入力終了

void Key_Read(void);

extern int  Key_PadConfig(INPUT_BITS Key);
extern void Key_PadConfig2(BYTE PadID, INPUT_BITS KeyData);

#endif
