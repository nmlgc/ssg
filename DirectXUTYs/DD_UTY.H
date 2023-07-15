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
#include "game/format_bmp.h"
#include "game/graphics.h"
#include "platform/buffer.h"
#include "platform/windows/surface_ddraw.h"
#include "DX_TYPE.H"					// 型＆マクロ
#include "DX_ERROR.H"					// エラー処理用関数
#include "MtoB.h"						// スクリーンキャプチャー
#include <array>
#include <variant>


// 定数＆マクロ //

// TODO: Move to GIAN_SURFACE_ID once LOADER.H does no longer depend on this
// header.
#define SURFACE_MAX		10				// テクスチャ,Prim,BackをのぞくSurfaceの最大数

#define XDDRAWENUM_MAX	4				// ビデオカード４枚差しをする人はいないと思うので...
#define XD3DENUM_MAX	8				// DDrawデバイスに対する3Dデバイスの列挙最大数

#define ALPHA_ONE		0x00			// 一種の加算α
#define ALPHA_NORM		0x01			// ノーマルなSrc-α

#define RGB256(r,g,b)	((r)+(g)*6+(b)*36+0x14)

// Bit depths and pixel formats //
// ---------------------------- //

// (We don't really care about the value of whatever variant type this holds,
// just about the type itself.)
struct PIXELFORMAT : public std::variant<uint8_t, uint16_t, uint32_t> {
	// Not worth auto-generating at all.
	using LARGEST = uint32_t;

	bool IsPalettized() const {
		return std::holds_alternative<uint8_t>(*this);
	}

	bool IsChanneled() const {
		return !IsPalettized();
	}

	std::strong_ordering operator <=>(const PIXELFORMAT& other) const {
		return (index() <=> other.index());
	}
};

class BITDEPTHS {
protected:
	// Since this class is directly serialized, an enum would be actively
	// harmful here.
	using ARRAY = std::array<uint8_t, 3>;

	// Sorted from the lowest to the highest one.
	constexpr static const ARRAY SUPPORTED = { 8, 16, 32 };
	static_assert(SUPPORTED[0] < SUPPORTED[1]);
	static_assert(SUPPORTED[1] < SUPPORTED[2]);

public:
	class BITDEPTH {
		uint8_t bpp;

		ARRAY::const_iterator find() const {
			return std::ranges::find(SUPPORTED, bpp);
		}

	public:
		constexpr BITDEPTH() :
			bpp(0) {
		}
		constexpr BITDEPTH(const BITDEPTH& other) = default;
		constexpr BITDEPTH& operator=(const BITDEPTH& other) = default;

		constexpr BITDEPTH(ARRAY::const_iterator it) :
			bpp((it >= SUPPORTED.end()) ? 0 : *it) {
		}

		explicit operator bool() const {
			return (find() != SUPPORTED.end());
		}

		uint8_t value() const {
			return bpp;
		}

		// Adding std::monostate to PIXELFORMAT would just make it awful to
		// use.
		std::optional<PIXELFORMAT> pixel_format() const {
			switch(bpp) {
			case  8:	return std::make_optional<PIXELFORMAT>(uint8_t{});
			case 16:	return std::make_optional<PIXELFORMAT>(uint16_t{});
			case 32:	return std::make_optional<PIXELFORMAT>(uint32_t{});
			default:	return std::nullopt;
			}
		}

		// Cycles through all supported bit depths.
		BITDEPTH cycle(bool reverse) const {
			const auto it = find();
			// Visual Studio raises a debug exception if an iterator is
			// decremented past begin()...
			if(it == SUPPORTED.end()) {
				return {};
			} else if(reverse && (it == SUPPORTED.begin())) {
				return (SUPPORTED.end() - 1);
			} else if(!reverse && (it == (SUPPORTED.end() - 1))) {
				return SUPPORTED.begin();
			}
			return (it + (reverse ? -1 : +1));
		}
	};

	static_assert(
		sizeof(BITDEPTH) == sizeof(uint8_t),
		"The BITDEPTH class is serialized and must be exactly 1 byte large."
	);

	constexpr static BITDEPTH find_if(auto pred) {
		return std::ranges::find_if(SUPPORTED, pred);
	}
};

using BITDEPTH = BITDEPTHS::BITDEPTH;
// ---------- //

// Forward declarations //
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
	HWND					hWnd;			// ウィンドウハンドル
	BOOL					bNeedChgPal;	// パレットを変更する必要があるか
	std::optional<uint8_t>	DeviceID;

protected:
	// Ensures that a DirectDraw surface matches its underlying image.
	bool SurfaceRestore(SURFACE_DDRAW& surf);

	// Also ensures the correct color key for palettized pixel formats.
	bool SurfaceSetColorKeyFromFirstPixel(SURFACE_DDRAW& surf);

public:
	// Can mutate the two parameters and replace them with a supported
	// configuration if necessary.
	bool Init(uint8_t& device_id, BITDEPTH& bitdepth);

	// いつも通りに(ただし失敗したら異常終了)
	void Cleanup();

	// ロード済みのSurfaceを修復
	bool Restore(void);

	// TODO: Remove once the loader no longer needs it.
	std::span<PALETTEENTRY> PaletteForDDraw();

	// オフスクリーン Surface 作成 //
	// --------------------------- //

	// Surface の作成(BMPリロード有り)
	bool SurfaceCreate(BYTE surf_id, PIXEL_SIZE size);

	// ＢＭＰファイルロード //
	bool SurfaceLoad(BYTE surf_id, BMP_OWNED bmp);

	// Mutates the top-left pixel of the DirectDraw (not GDI) surface.
	bool SurfaceSetColorKey(SURFACE_DDRAW& surf, RGBA col);
	// --------------------------- //
};

typedef struct tagD3DENUM{
	LPGUID		lpGuid;					// GUID
	char		*name;					// お名前
	BOOL		bIsHW;					// ハードウェアか
	BOOL		b8Bit;					// この D3D_DEVICE が  8Bit に対応しているか
	BOOL		b16Bit;					// この D3D_DEVICE が 16Bit に対応しているか
	BOOL		b24Bit;					// この D3D_DEVICE が 24Bit に対応しているか
	BOOL		b32Bit;					// この D3D_DEVICE が 32Bit に対応しているか
	DWORD		Zbuf;					// サポートするＺバッファの深度(０は多分無いのでしょう)
} XD3DENUM;

typedef struct tagDDRAWENUM{
	LPGUID		lpDDGuid;				// あるドライバのGUID
	char		*name;					// お名前
	XD3DENUM	D3D[XD3DENUM_MAX];		// D3D_DEVICE の GUID & ドライバ名
	DWORD		D3DNow;					// D3D_DEVICE の現在の数
	BOOL		b8Bit;					// この DD_DEVICE が  8Bit に対応しているか
	BOOL		b16Bit;					// この DD_DEVICE が 16Bit に対応しているか
	BOOL		b24Bit;					// この DD_DEVICE が 24Bit に対応しているか
	BOOL		b32Bit;					// この DD_DEVICE が 32Bit に対応しているか

	// Only looks at the first D3D device?
	bool BitDepthSupported(uint8_t bpp) const {
		switch(bpp) {
		case  8:	return b8Bit;
		case 16:	return (b16Bit && D3D->b16Bit);
		case 32:	return (b32Bit && D3D->b32Bit);
		default:	return false;
		}
	}

	bool BitDepthSupported(const BITDEPTH& bitdepth) const {
		return BitDepthSupported(bitdepth.value());
	}

	auto BitDepthBest() const {
		return BITDEPTHS::find_if([this](auto bpp) {
			return BitDepthSupported(bpp);
		});
	}
} XDDRAWENUM;


// グローバル変数 //
extern GRAPHICS_DDRAW	DxObj;
extern SURFACE_DDRAW	DxSurf[];
extern XDDRAWENUM	DxEnum[XDDRAWENUM_MAX];
extern DWORD		DxEnumNow;


// 初期化など //
bool GrpEnum(HWND hWnd);	// 使用できるデバイスを列挙する
void GrpSetClip(int sx, int sy, int ex, int ey);	// 3DViewportが生きていたらそちらもセット

void GrpGetPalette(PALETTE& pal);	// 現在のパレットを取得する
void GrpSetPalette(const PALETTE& pal);	// 現在のパレットを変更する
void GrpSetRestoreHook(bool (*func)(void));	// Restore の事後処理フック

// 図形描画関連関数Ptr(GrpInitにより初期化される) //
void GrpCls(void);	// 指定色で塗りつぶしをかける(クリッピング無効)
void GrpClsEx(const std::optional<WINDOW_LTRB>& Target, DWORD col);	// 直接番号指定で矩形内を塗りつぶす

extern void (*GrpFlip)(void);	// ウィンドウモード時の動作に注意
extern void (*GrpLock)(void);	// 図形描画の準備をする
extern void (*GrpUnlock)(void);	// 図形描画を完了する

extern void (*GrpSetColor)(uint8_t r, uint8_t g, uint8_t b);	// 色セット
extern void (*GrpSetAlpha)(uint8_t a, uint8_t mode);	// αセット

extern void (*GrpLine)(int x1, int y1, int x2, int y2);	// 直線
extern void (*GrpBox)(int x1, int y1, int x2, int y2);	// 長方形
extern void (*GrpCircle)(int x, int y, int r);	// 円
extern void (*GrpCircleF)(int x, int y, int r);	// 円塗りつぶし
extern void (*GrpPolygon)(WINDOW_POINT *p, int n);	// ポリゴン塗りつぶし

extern void (*GrpBoxA)(int x1, int y1, int x2, int y2);	// α長方形
extern void (*GrpCircleA)(int x, int y, int r);	// α円塗りつぶし
extern void (*GrpPolygonA)(WINDOW_POINT *p, int n);	// αポリゴン塗りつぶし

extern void (*GrpFatCircleA)(int x, int y, int r, int w);	// 太い円を描く

// ビットマップ転送(WithClip)
// Returns `true` if any part of the sprite was blitted.
extern bool(*GrpBlt)(PIXEL_LTRB *src, int x, int y, SURFACE_DDRAW& surf);

extern bool GrpClip(PIXEL_LTRB *src, int *x, int *y);	// クリッピングをかける
extern void GrpGetClip(WINDOW_LTRB *dest);	// クリッピング枠を取得する

// カラーキー付き転送(クリッピングはないので注意！！) //
// src の値は変更されません //
// Always returns `true`. //
bool GrpBltX(PIXEL_LTRB *src, int x, int y, SURFACE_DDRAW& surf);

// カラーキー無し転送(クリッピングはないので注意！！) //
// src の値は変更されません //
void GrpBltXC(PIXEL_LTRB *src, int x, int y, SURFACE_DDRAW& surf);

#endif
