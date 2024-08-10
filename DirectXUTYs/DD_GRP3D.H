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
import std.compat;
#include "game/coords.h"
#include "game/graphics.h"


// 関数 //
void _3DSetup(void);	// 描画系関数ポインタをセットする

void _3DLock(void);	// 画面描画の準備をする
void _3DUnlock(void);	// 画面描画を完了する

void _3DSetColor(RGB216 col);	// 色セット
void _3DSetAlpha(uint8_t a, uint8_t mode);	// αセット

void _3DLine(int x1, int y1, int x2, int y2);	// 直線
void _3DBox(int x1, int y1, int x2, int y2);	// 長方形
void _3DCircle(int x, int y, int r);	// 円
void _3DCircleF(int x, int y, int r);	// 円塗りつぶし
void _3DPolygon(std::span<const WINDOW_POINT> p);	// ポリゴン塗りつぶし
void _3DBoxA(int x1, int y1, int x2, int y2);	// α長方形


// ３Ｄ専用関数 //
void GrpCircleFA(int x, int y, int r);	// α円塗りつぶし
void GrpFatCircleA(int x, int y, int r, int w);	// 太い円を描く
void GrpPolygonA(std::span<const WINDOW_POINT> p);	// αポリゴン塗りつぶし
void GrpGrdRect(std::span<const WINDOW_POINT, 4> p);	// グラデーション付き長方形(ナナメ可)
void GrpGrdRectA(std::span<const WINDOW_POINT, 4> p);	// グラデーション付き長方形(ナナメ可+α)
void GrpGrdPolygonA(std::span<const WINDOW_POINT> p);	// グラデーション付きポリゴン(レーザー用？)
void GrpGrdLineEx(int x, int y1, int y2);	// スペアな用グラデーションライン


#endif
