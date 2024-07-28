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
#include "game/coords.h"
#include "DirectXUTYs/DD_GRP2D.H"
#include "DirectXUTYs/DD_GRP3D.H"
#include "platform/windows/surface_ddraw.h"


// 定数＆マクロ //

// TODO: Move to GIAN_SURFACE_ID once LOADER.H does no longer depend on this
// header.
#define SURFACE_MAX		10				// テクスチャ,Prim,BackをのぞくSurfaceの最大数

// Forward declarations //
struct BMP_OWNED;
struct IDirectDraw2;
struct IDirectDrawPalette;
struct IDirectDrawSurface;
struct IDirect3D2;
struct IDirect3DDevice2;
struct IDirect3DViewport2;

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
	std::optional<uint8_t>	DeviceID;

protected:
	// Ensures that a DirectDraw surface matches its underlying image.
	bool SurfaceRestore(SURFACE_DDRAW& surf);

	// Also ensures the correct color key for palettized pixel formats.
	bool SurfaceSetColorKeyFromFirstPixel(SURFACE_DDRAW& surf);

	bool SurfaceUpdateFromGDI(SURFACE_DDRAW& surf, const PIXEL_LTWH& r);

public:
	// Can mutate the two parameters and replace them with a supported
	// configuration if necessary.
	bool Init(uint8_t& device_id, BITDEPTH& bitdepth);

	// いつも通りに(ただし失敗したら異常終了)
	void Cleanup();

	// ロード済みのSurfaceを修復
	bool Restore(void);

	// オフスクリーン Surface 作成 //
	// --------------------------- //

	// (Re-)creates the DirectDraw surface associated with [surf_id], in the
	// given size and with undefined initial contents, retaining whatever state
	// the GDI surface might have had. After filling the GDI surface with the
	// intended pixels, call SurfaceRestore() to upload them to the DirectDraw
	// surface.
	bool SurfaceCreateUninitialized(SURFACE_DDRAW& surf, PIXEL_SIZE size);

	// (Re-)creates the GDI and DirectDraw surfaces associated with [surf], in
	// the given size and with undefined initial contents. After filling the
	// GDI surface with the intended pixels, call SurfaceRestore() to upload
	// them to the DirectDraw one.
	bool SurfaceCreateBlank(SURFACE_DDRAW& surf, PIXEL_SIZE size);

	// Consumes the given .BMP file and sets the given surface to its contents,
	// re-creating it in the correct size if necessary.
	bool SurfaceLoad(uint8_t surf_id, BMP_OWNED&& bmp);

	// Mutates the top-left pixel of the DirectDraw (not GDI) surface.
	bool SurfaceSetColorKey(SURFACE_DDRAW& surf, RGB col);

	// Ensures that the given rectangle of the DirectDraw surface matches the
	// underlying GDI image after an edit.
	template <typename T> bool SurfaceEdit(
		SURFACE_DDRAW& surf,
		T func,
		std::optional<PIXEL_LTWH> rect = std::nullopt
	) {
		const auto r = rect.value_or(
			PIXEL_LTWH{ 0, 0, surf.size.w, surf.size.h }
		);
		return (func(surf.dc, r) && SurfaceUpdateFromGDI(surf, r));
	}

	// Blits the given [src] rectangle inside [surf] to the given top-left
	// point on the back buffer, while clipping the destination rectangle to
	// the clipping area. Returns `true` if any part of the sprite was blitted.
	bool SurfaceBlit(
		WINDOW_POINT topleft, SURFACE_DDRAW& surf, PIXEL_LTRB& src
	);
	// --------------------------- //
};


// グローバル変数 //
extern GRAPHICS_DDRAW	DxObj;
extern GRAPHICS_GEOMETRY *GrpGeom;
extern SURFACE_DDRAW	DxSurf[SURFACE_MAX];


// 初期化など //
bool GrpEnum(void);	// 使用できるデバイスを列挙する
void GrpSetClip(int sx, int sy, int ex, int ey);	// 3DViewportが生きていたらそちらもセット

void GrpGetPalette(PALETTE& pal);	// 現在のパレットを取得する
bool GrpSetPalette(const PALETTE& pal);	// 現在のパレットを変更する
void GrpSetPaletteFrom(SURFACE_DDRAW& surf);

// ビットマップ転送(WithClip)
// Returns `true` if any part of the sprite was blitted.
extern bool(*GrpBlt)(PIXEL_LTRB *src, int x, int y, SURFACE_DDRAW& surf);

extern void GrpGetClip(WINDOW_LTRB *dest);	// クリッピング枠を取得する

// カラーキー付き転送(クリッピングはないので注意！！) //
// src の値は変更されません //
// Always returns `true`. //
bool GrpBltX(const PIXEL_LTRB& src, int x, int y, SURFACE_DDRAW& surf);

// カラーキー無し転送(クリッピングはないので注意！！) //
// src の値は変更されません //
void GrpBltXC(const PIXEL_LTRB& src, int x, int y, SURFACE_DDRAW& surf);

#endif
