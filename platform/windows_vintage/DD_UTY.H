/*                                                                           */
/*   DD_UTY.h   描画関連の関数                                               */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_DD_UTY_H
#define PBGWIN_DD_UTY_H		"DD_UTY : Version 0.07 : Update 2000/02/03"


// 更新履歴 //

// 2000/08/21 : クリッピング関数を更新・追加

// 2000/03/01 : GrpSetRestoreHook() を追加
// 2000/02/28 : GrpFatCircleA() を追加。使い方次第では便利なのでは？

// 1999/12/05 : クリッピング描画を追加(一通りの関数が揃う)
// 1999/12/02 : 列挙系の関数がほぼ完成する。ＢＭＰローダも後少しで完成するはず
//            : 関数ポインタの追加
// 1999/11/30 : D3D_IM を使用するように変更。それに伴い、全体的に大幅な変更あり
//            : また、D3DRM_UTY を削除した。


// ヘッダファイル //
#include "DD_GRP2D.H"	// ２Ｄ用描画関数(公開しないこと)
#include "DD_GRP3D.H"	// ３Ｄ用描画関数(公開しないこと)
#include "game/graphics.h"


// Forward declarations //
struct BMP_OWNED;
struct IDirectDraw2;
struct IDirectDrawPalette;
struct IDirectDrawSurface;
struct IDirect3D2;
struct IDirect3DDevice2;
struct IDirect3DViewport2;
struct SURFACE_DDRAW;

// 構造体 //
struct GRAPHICS_DDRAW {
	IDirectDraw2*	DD;	// DirectDraw Object
	IDirect3D2*	D3;
	IDirect3DDevice2*	D3Dev;
	IDirect3DViewport2*	View;

	IDirectDrawPalette*	Pal;	// パレット(8Bitのときだけ)

	IDirectDrawSurface*	Prim;	// 表画面
	IDirectDrawSurface*	Back;	// 裏画面
	IDirectDrawSurface*	Zbuf;	// Ｚバッファ

	PALETTE	Palette;

	PIXELFORMAT PixelFormat;
	bool	bNeedChgPal;	// パレットを変更する必要があるか

public:
	// Ensures that a DirectDraw surface matches its underlying image.
	bool SurfaceRestore(SURFACE_DDRAW& surf);

	// Also ensures the correct color key for palettized pixel formats.
	bool SurfaceSetColorKeyFromFirstPixel(SURFACE_DDRAW& surf);

	// ロード済みのSurfaceを修復
	bool Restore(void);

	// (Re-)creates the DirectDraw surface associated with [surf_id], in the
	// given size and with undefined initial contents, retaining whatever state
	// the GDI surface might have had. After filling the GDI surface with the
	// intended pixels, call SurfaceRestore() to upload them to the DirectDraw
	// surface.
	bool SurfaceCreateUninitialized(SURFACE_DDRAW& surf, PIXEL_SIZE size);

	// Mutates the top-left pixel of the DirectDraw (not GDI) surface.
	bool SurfaceSetColorKey(SURFACE_DDRAW& surf, RGB col);
};


// グローバル変数 //
extern GRAPHICS_DDRAW	DxObj;
extern GRAPHICS_GEOMETRY *GrpGeom;

#define GRP_SUPPORT_BITDEPTH
#undef GRP_SUPPORT_SCALING
#undef GRP_SUPPORT_API

#endif
