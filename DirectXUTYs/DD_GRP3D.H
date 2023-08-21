/*                                                                           */
/*   DD_GRP3D.h   ３Ｄ時の図形描画関数                                       */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_DD_GRP3D_H
#define PBGWIN_DD_GRP3D_H		"DD_GRP3D : Version 0.03 : Update 2000/02/22"


// 更新履歴 //

// 2000/02/22 : GrpGrdPolygon() を追加
// 1999/12/04 : レーザー用特殊α関数を追加
// 1999/12/02 : やっと一部の関数が動作し始めた。16Bit-αはやっぱり綺麗だな...


// ヘッダファイル //
#include "game/coords.h"
#include <stdint.h>


// 関数 //
void _3DSetup(void);	// 描画系関数ポインタをセットする

void _3DLock(void);	// 画面描画の準備をする
void _3DUnlock(void);	// 画面描画を完了する

void _3DSetColor(uint8_t r, uint8_t g, uint8_t b);	// 色セット
void _3DSetAlpha(uint8_t a, uint8_t mode);	// αセット

void _3DLine(int x1, int y1, int x2, int y2);	// 直線
void _3DBox(int x1, int y1, int x2, int y2);	// 長方形
void _3DCircle(int x, int y, int r);	// 円
void _3DCircleF(int x, int y, int r);	// 円塗りつぶし
void _3DPolygon(WINDOW_POINT *p, int n);	// ポリゴン塗りつぶし

void _3DBoxA(int x1, int y1, int x2, int y2);	// α長方形
void _3DCircleA(int x, int y, int r);	// α円塗りつぶし
void _3DPolygonA(WINDOW_POINT *p, int n);	// αポリゴン塗りつぶし
void _3DFatCircleA(int x, int y, int r, int w);	// 太い半透明円描画


// ３Ｄ専用関数 //
void GrpGrdRect(WINDOW_POINT *p);	// グラデーション付き長方形(ナナメ可)
void GrpGrdRectA(WINDOW_POINT *p);	// グラデーション付き長方形(ナナメ可+α)
void GrpGrdPolygon(WINDOW_POINT *p, int n);	// グラデーション付きポリゴン(レーザー用？)
void GrpGrdLineEx(int x, int y1, int y2);	// スペアな用グラデーションライン


#endif
