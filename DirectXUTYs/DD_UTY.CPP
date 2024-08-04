/*                                                                           */
/*   DD_UTY.cpp   描画関連の関数                                             */
/*                                                                           */
/*                                                                           */

#include "DD_UTY.H"
#include "constants.h"
#include "game/cast.h"
#include "game/debug.h"
#include "game/format_bmp.h"
#include "game/input.h"
#include <assert.h>
#include <d3d.h>
#pragma message(PBGWIN_DD_UTY_H)

#pragma comment(lib, "ddraw.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "user32.lib")

#if   DIRECTDRAW_VERSION == 0x0700
	#pragma message("      DirectDraw  : Version 7.00")
#elif DIRECTDRAW_VERSION == 0x0600
	#pragma message("      DirectDraw  : Version 6.00")
#elif DIRECTDRAW_VERSION == 0x0500
	#pragma message("      DirectDraw  : Version 5.00")
#elif DIRECTDRAW_VERSION == 0x0300
	#pragma message("      DirectDraw  : Version 3.00")
#endif


// マクロ //
void RELEASE(auto*& x)
{
	if(x != nullptr) {
		x->Release();
		x = nullptr;
	}
}

// 図形描画関数に対するダミー関数 //
static bool DM_GRPBLT(PIXEL_LTRB *src, int x, int y, SURFACE_DDRAW& surf);

// クリッピング枠 //
int CLIP_X_MIN = 0;
int CLIP_Y_MIN = 0;
int CLIP_X_MAX = (GRP_RES.w - 1);
int CLIP_Y_MAX = (GRP_RES.h - 1);

static constexpr bool Fullscreen = true;

bool STD_GrpBlt(PIXEL_LTRB *src, int x, int y, SURFACE_DDRAW& surf);


// Ｚバッファを必要なら生成し、接続する //
static bool CreateZbuffer(const XD3DENUM& D3De);

// 列挙用コールバック関数群 //
static BOOL FAR __stdcall DriverEnum(GUID FAR *lpGUID,LPSTR lpDDesc,LPSTR lpDName,LPVOID lpContext);
static HRESULT FAR __stdcall DeviceEnum(LPGUID lpGUID,LPSTR lpDeviceDesc,LPSTR lpdeviceName,LPD3DDEVICEDESC lpHWDesc,LPD3DDEVICEDESC lpHELDesc,LPVOID lpContext);
static HRESULT FAR __stdcall ModeEnum(DDSURFACEDESC *pddsd,VOID *pInfo);

// デバイスが求める性能を持っているか //
static bool ConfirmDevice3D(const D3DDEVICEDESC *lpDesc);


// グローバル変数 //
GRAPHICS_DDRAW	DxObj;
GRAPHICS_GEOMETRY *GrpGeom;
GRAPHICS_GEOMETRY_DDRAW GrpGeom_DDraw;
GRAPHICS_GEOMETRY_D3D GrpGeom_D3D;
SURFACE_DDRAW	DxSurf[SURFACE_MAX];
XDDRAWENUM		DxEnum[XDDRAWENUM_MAX];
uint8_t DxEnumNow;

// 図形描画用関数ポインタの実体 //
bool(*GrpBlt)(PIXEL_LTRB *src, int x, int y, SURFACE_DDRAW& surf) = STD_GrpBlt;

// Templating the functor avoids both the need for #including <functional> and
// any potential runtime exceptions related to the call.
void DDrawRetryUntilSuccessful(GRAPHICS_DDRAW& dxobj, auto action)
{
	for(;;) {
		const HRESULT ddrval = action();
		if(ddrval == DD_OK) {
			break;
		}
		if(ddrval == DDERR_SURFACELOST) {
			if(!dxobj.Restore()) {
				return;
			}
		}
		if(ddrval != DDERR_WASSTILLDRAWING) {
			break;
		}
	}
}


bool GrpEnum(void)
{
	HRESULT		dxret;

	ZeroMemory(DxEnum, sizeof(DxEnum));
	DxEnumNow = 0;

	// 列挙開始 //
	dxret = DirectDrawEnumerate(DriverEnum,NULL);
	if(dxret != DD_OK) {
		DebugOut(u8"DirectDrawEnumerate に失敗");
		return false;
	}

	extern HWND hWndMain;
	DxObj.hWnd = hWndMain;

#ifdef _DEBUG
	DebugOut(u8"正常に列挙できたの");
#endif

	return TRUE;
}

uint8_t GrpBackend_DeviceCount(void)
{
	return DxEnumNow;
}

Any::string_view GrpBackend_DeviceName(uint8_t id)
{
	if(id >= DxEnumNow) {
		return {};
	}
	return DxEnum[id].name;
}

bool GRAPHICS_DDRAW::Init(uint8_t& device_id, BITDEPTH& bitdepth)
{
	HRESULT				dxret;
	LPDIRECTDRAW		lpDD  = NULL;
	DDSURFACEDESC		ddsd;
//	D3DMATERIAL			mat;
//	D3DMATERIALHANDLE	hmat;

	if(hWnd == nullptr) {
		return false;
	}

	if(device_id >= DxEnumNow) {
		device_id = 0;
	}
	auto& pXDD = DxEnum[device_id];

	// Change bit depth to a supported one, if necessary //
	if(!pXDD.BitDepthSupported(bitdepth)) {
		bitdepth = pXDD.BitDepthBest();
		if(!bitdepth) {
			// TODO: Check other devices.
			return false;
		}
	}

	const auto maybe_pixel_format = bitdepth.pixel_format();
	if(!maybe_pixel_format) {
		return false;
	}
	const auto pixel_format = maybe_pixel_format.value();

	// Already running with this configuration? //
	if((device_id == DeviceID) && (pixel_format == PixelFormat)) {
		return false;
	}

	Cleanup();
	ZeroMemory(&ddsd, sizeof(ddsd));

	try{
		dxret = DirectDrawCreate(pXDD.lpDDGuid, &lpDD, nullptr);
		if(dxret!=DD_OK) throw("DirectDrawCreate に失敗");

		auto** dd_ptr = reinterpret_cast<void **>(&DD);
		dxret = lpDD->QueryInterface(IID_IDirectDraw2, dd_ptr);
		if(dxret!=DD_OK) throw("QueryInterface に失敗");
		RELEASE(lpDD);

		// 強調レベルのセット //
		dxret = DD->SetCooperativeLevel(
			hWnd, (DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN)
		);
		if(dxret!=DD_OK) throw("SetCooperativeLevel に失敗");

		// ディスプレイモードのセット //
		dxret = DD->SetDisplayMode(
			GRP_RES.w, GRP_RES.h, bitdepth.value(), 0, 0
		);
		if(dxret!=DD_OK) throw("SetDisplayMode に失敗");

		// プライマリ＆バックサーフェスの作成 //
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS|DDSD_BACKBUFFERCOUNT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE|DDSCAPS_3DDEVICE|DDSCAPS_FLIP|DDSCAPS_COMPLEX;
		ddsd.dwBackBufferCount = 1;
		dxret = DD->CreateSurface(&ddsd, &Prim, nullptr);
		if(dxret!=DD_OK) throw("CreateSurface(Prim) に失敗");

		// BackBuffer へのポインタを Get //
		DDSCAPS ddscaps = { .dwCaps = DDSCAPS_BACKBUFFER };
		dxret = Prim->GetAttachedSurface(&ddscaps, &Back);
		if(dxret!=DD_OK) throw("GetAttachedSurface に失敗");

		// 2Dの初期化はここでおしまい //
		DeviceID = device_id;
		PixelFormat = pixel_format;
		if(PixelFormat.IsPalettized()) throw(true);

		// Direct3D Object の取得 //
		auto** d3d_ptr = reinterpret_cast<void **>(&D3);
		dxret = DD->QueryInterface(IID_IDirect3D2, d3d_ptr);
		if(dxret!=DD_OK) throw("QueryInterface に失敗");

		// Direct3D Device の作成 //
		dxret = D3->CreateDevice(*pXDD.D3D[0].lpGuid, Back, &D3Dev);
		//dxret = D3->CreateDevice(IID_IDirect3DHALDevice, Back, &D3Dev);
		if(dxret!=D3D_OK) throw("CreateDevice(D3D_Device) に失敗");

		// Ｚバッファ作成＆接続 //
		if(!CreateZbuffer(pXDD.D3D[0]))
			throw("CreateSurface(Z-Buffer) に失敗");

		// Direct3D Viewport の作成＆接続 //
		dxret = D3->CreateViewport(&View, nullptr);
		if(dxret!=D3D_OK) throw("CreateViewport に失敗");
		dxret = D3Dev->AddViewport(View);
		if(dxret!=D3D_OK) throw("AddViewport に失敗");
		dxret = D3Dev->SetCurrentViewport(View);
		if(dxret!=D3D_OK) throw("SetCurrentViewport に失敗");

		throw(true);
	}
	catch(const std::u8string_view error) {
		DebugOut(error);
		Cleanup();
		return FALSE;
	}
	catch(const bool flag) {
		if(pixel_format.IsPalettized()) {
			GrpGeom = &GrpGeom_DDraw;
			GrpGeom_DDraw.Setup();
		} else {
			GrpGeom = &GrpGeom_D3D;
		}

		GrpSetClip(0, 0, GRP_RES.w, GRP_RES.h);

		for(auto& surf : DxSurf) {
			if(surf.size) {
				SurfaceCreateUninitialized(surf, surf.size);
				SurfaceRestore(surf);
			}
		}

		return flag;
	}

	return FALSE;
}

void GRAPHICS_DDRAW::Cleanup(void)
{
	RELEASE(View);
	RELEASE(D3Dev);
	RELEASE(D3);
	RELEASE(Zbuf);

	RELEASE(Pal);

	// ここではSurfaceのBMPを解放しないことに注意 //
	for(auto& surf : DxSurf) {
		RELEASE(surf.surf);
	}

	RELEASE(Back);
	RELEASE(Prim);
	RELEASE(DD);
}

PIXELFORMAT GrpBackend_PixelFormat(void)
{
	return DxObj.PixelFormat;
}

// クリッピング枠を取得する //
void GrpGetClip(WINDOW_LTRB *dest)
{
	dest->left   = CLIP_X_MIN;
	dest->top    = CLIP_Y_MIN;

	dest->right  = CLIP_X_MAX;
	dest->bottom = CLIP_Y_MAX;
}

void GrpSetClip(int sx, int sy, int ex, int ey)
{
	D3DVIEWPORT2		view;

	CLIP_X_MIN = (sx<0) ? 0 : sx;
	CLIP_Y_MIN = (sy<0) ? 0 : sy;
	CLIP_X_MAX = (std::min)(ex, (GRP_RES.w - 1));
	CLIP_Y_MAX = (std::min)(ey, (GRP_RES.h - 1));

	if ((CLIP_X_MIN == CLIP_X_MAX) || (CLIP_Y_MIN == CLIP_Y_MAX)) {
		GrpBlt = DM_GRPBLT;
	} else {
		GrpBlt = STD_GrpBlt;
	}

	if(DxObj.View){
		view.dwSize = sizeof(view);
		view.dwX          = CLIP_X_MIN;
		view.dwY          = CLIP_Y_MIN;
		view.dwWidth      = (CLIP_X_MAX - CLIP_X_MIN + 1);
		view.dwHeight     = (CLIP_Y_MAX - CLIP_Y_MIN + 1);
		view.dvClipX      = D3DVAL(1);
		view.dvClipY      = D3DVAL(1);
		view.dvClipWidth  = D3DVAL(2);
		view.dvClipHeight = D3DVAL(2);
		view.dvMinZ       = D3DVAL(0);
		view.dvMaxZ       = D3DVAL(1);
		DxObj.View->SetViewport2(&view);
	}
}

bool GRAPHICS_DDRAW::Restore(void)
{
	if(Prim) {
		const auto dxval = Prim->Restore();
		if(dxval!=DD_OK) return FALSE;
	}
	if(Back) {
		const auto dxval = Back->Restore();
		if(dxval!=DD_OK) return FALSE;
	}
	if(Zbuf) {
		const auto dxval = Zbuf->Restore();
		if(dxval!=DD_OK) return FALSE;
	}

	for(auto& surf : DxSurf) {
		if(!SurfaceRestore(surf)) {
			return false;
		}
	}

	return true;
}

std::span<PALETTEENTRY> CastToPALETTEENTRY(PALETTE& pal)
{
	// Verify that PALETTEENTRY and RGBA are identical.
	using COLOR = std::decay<decltype(pal)>::type::value_type;
	static_assert(sizeof(COLOR) == sizeof(PALETTEENTRY));
	static_assert(offsetof(COLOR, r) == offsetof(PALETTEENTRY, peRed));
	static_assert(offsetof(COLOR, g) == offsetof(PALETTEENTRY, peGreen));
	static_assert(offsetof(COLOR, b) == offsetof(PALETTEENTRY, peBlue));
	static_assert(offsetof(COLOR, a) == offsetof(PALETTEENTRY, peFlags));
	return { reinterpret_cast<PALETTEENTRY *>(pal.data()), pal.size() };
}

// 現在のパレットを取得する(ただの代入なのでこの関数を使うまでも無いかもしれないね) //
void GrpGetPalette(PALETTE& pal)
{
	// ８Ｂｉｔモード以外では、何もしないでリターンする //
	if(DxObj.PixelFormat.IsChanneled()) return;

	pal = DxObj.Palette;
}

bool GrpSetPalette(const PALETTE& pal)
{
	// ８Ｂｉｔモード以外では、何もしないでリターンする //
	if(DxObj.PixelFormat.IsChanneled()) {
		return true;
	}

	DxObj.Palette = pal;
	DxObj.Palette[0] = { 0, 0, 0 };	// 強制的に色をセットしてしまう //

	// パレットを変更
	const auto p = CastToPALETTEENTRY(DxObj.Palette);
	if(DxObj.Pal) {
		DxObj.Pal->SetEntries(0, 0, p.size(), p.data());
	} else {
		const auto ddrval = DxObj.DD->CreatePalette(
			(DDPCAPS_8BIT | DDPCAPS_ALLOW256), p.data(), &DxObj.Pal, nullptr
		);
		if(ddrval != DD_OK) {
			return false;
		}
	}
	DxObj.Prim->SetPalette(DxObj.Pal);
	DxObj.bNeedChgPal = TRUE;

	DisableAlphaTable();
	return true;
}

void GrpSetPaletteFrom(SURFACE_DDRAW& surf)
{
	if(DxObj.PixelFormat.IsChanneled()) {
		return;
	}

	LPDIRECTDRAWPALETTE lpdp;
	if(surf.surf->GetPalette(&lpdp) != DD_OK) {
		return;
	}

	const auto p = CastToPALETTEENTRY(DxObj.Palette);
	lpdp->GetEntries(0, 0, p.size(), p.data());
	GrpSetPalette(DxObj.Palette);

	lpdp->Release();
}

bool GRAPHICS_DDRAW::SurfaceCreateUninitialized(
	SURFACE_DDRAW& surf, PIXEL_SIZE size
)
{
	DDSURFACEDESC		ddsd;
	IDirectDrawSurface*	temp = nullptr;

	RELEASE(surf.surf);
	surf.size = { 0, 0 };

	ZeroMemory(&ddsd, sizeof(ddsd));

	ddsd.dwSize         = sizeof(ddsd);
	ddsd.dwWidth        = size.w;
	ddsd.dwHeight       = size.h;
	ddsd.dwFlags        = DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	const auto dxval = DD->CreateSurface(&ddsd, &temp, nullptr);
	if(dxval != DD_OK) {
		return false;
	}
	surf.size = size;
	surf.surf = temp;
	return true;
}

bool GRAPHICS_DDRAW::SurfaceCreateBlank(SURFACE_DDRAW& surf, PIXEL_SIZE size)
{
	surf.Delete();

	if(
		!size ||
		(size.w > (std::numeric_limits<int32_t>::max)()) ||
		(size.h > (std::numeric_limits<int32_t>::max)())
	) {
		assert(!"Invalid size for blank surface");
		return false;
	}

	HDC reference_dc;
	if(Back->GetDC(&reference_dc) != DD_OK) {
		return false;
	}

	// This *MUST* be a compatible bitmap. You do *NOT* want to blit a 24-bit
	// GDI image onto an 8-bit DirectDraw surface, and work around all of GDI's
	// undocumented palette color matching quirks.
	surf.img = CreateCompatibleBitmap(reference_dc, size.w, size.h);

	if((Back->ReleaseDC(reference_dc) != DD_OK) || !surf.img) {
		return false;
	}
	surf.stock_img = SelectObject(surf.dc, surf.img);
	return SurfaceCreateUninitialized(surf, size);
}

bool GRAPHICS_DDRAW::SurfaceLoad(BYTE surf_id, BMP_OWNED&& bmp)
{
	if(surf_id>=SURFACE_MAX) return FALSE;

	auto& surf = DxSurf[surf_id];
	const PIXEL_SIZE bmp_size = {
		std::abs(bmp.info.biWidth), std::abs(bmp.info.biHeight),
	};
	if(surf.size != bmp_size) {
		if(!SurfaceCreateUninitialized(surf, bmp_size)) {
			return false;
		}
	}
	return (surf.Load(std::move(bmp)) && SurfaceRestore(surf));
}

bool GRAPHICS_DDRAW::SurfaceSetColorKey(SURFACE_DDRAW& surf, RGB col)
{
	HDC ddraw_dc;
	return (
		surf.surf &&
		(surf.surf->GetDC(&ddraw_dc) == DD_OK) &&
		SetPixelV(ddraw_dc, 0, 0, RGB(col.r, col.g, col.b)) &&
		(surf.surf->ReleaseDC(ddraw_dc) == DD_OK) &&
		SurfaceSetColorKeyFromFirstPixel(surf)
	);
}

bool GRAPHICS_DDRAW::SurfaceBlit(
	WINDOW_POINT topleft, SURFACE_DDRAW& surf, PIXEL_LTRB& src
)
{
	return GrpBlt(&src, topleft.x, topleft.y, surf);
}

void GrpBackend_Clear(uint8_t col_palettized, RGB col_channeled)
{
	DDBLTFX		ddbltfx;

	ZeroMemory(&ddbltfx,sizeof(ddbltfx));
	ddbltfx.dwSize      = sizeof(ddbltfx);
	ddbltfx.dwFillColor = (DxObj.PixelFormat.IsPalettized()
		? col_palettized
		: RGB(col_channeled.r, col_channeled.g, col_channeled.b)
	);

	DDrawRetryUntilSuccessful(DxObj, [&] {
		return DxObj.Back->Blt(
			nullptr, nullptr, nullptr, DDBLT_COLORFILL, &ddbltfx
		);
	});
}

GRAPHICS_GEOMETRY_D3D *GrpGeom_Poly(void)
{
	return (DxObj.PixelFormat.IsChanneled() ? &GrpGeom_D3D : nullptr);
}

GRAPHICS_GEOMETRY_DDRAW *GrpGeom_FB(void)
{
	return (DxObj.PixelFormat.IsPalettized() ? &GrpGeom_DDraw : nullptr);
}


void GrpBackend_Flip(std::unique_ptr<FILE_STREAM_WRITE> screenshot_stream)
{
	if(auto *stream = screenshot_stream.get()) {
		DDrawSaveSurface(stream, DxObj.Back);
	}

	if(Fullscreen) {
		// パレットを変更する必要があれば、変更だ //
		if(DxObj.bNeedChgPal && DxObj.PixelFormat.IsPalettized()) {
			DxObj.Back->SetPalette(DxObj.Pal); // 裏画面にパレットをセットする
			DxObj.bNeedChgPal = FALSE;
		}
		DDrawRetryUntilSuccessful(DxObj, [&] {
			return DxObj.Prim->Flip(nullptr, 0);
		});
	} else {
		DDrawRetryUntilSuccessful(DxObj, [&] {
			return DxObj.Prim->Blt(nullptr, DxObj.Back, nullptr, 0, nullptr);
		});
	}
	screenshot_stream.reset();
}

// クリッピングをかける
bool GrpClip(PIXEL_LTRB *src, int *x, int *y)
{
	const PIXEL_COORD w = (src->right - src->left - 1);
	const PIXEL_COORD h = (src->bottom - src->top - 1);

	// 完全に外かな？ //
	if((*x)>CLIP_X_MAX||(*y)>CLIP_Y_MAX||(*x)+w<CLIP_X_MIN||(*y)+h<CLIP_Y_MIN) return FALSE;

	// Ｘクリッピング //
	if((*x)<CLIP_X_MIN){
		if((*x)+w>CLIP_X_MAX)
			src->right -= ((*x)+w - CLIP_X_MAX);
		src->left += (CLIP_X_MIN - (*x));
		(*x) = CLIP_X_MIN;
	}
	else if((*x)+w>CLIP_X_MAX)
		src->right -= ((*x)+w - CLIP_X_MAX);

	// Ｙクリッピング //
	if((*y)<CLIP_Y_MIN){
		if((*y)+h>CLIP_Y_MAX)
			src->bottom -= ((*y)+h - CLIP_Y_MAX);
		src->top += (CLIP_Y_MIN - (*y));
		(*y) = CLIP_Y_MIN;
	}
	else if((*y)+h>CLIP_Y_MAX)
		src->bottom -= ((*y)+h - CLIP_Y_MAX);

	return TRUE;
}

bool GrpBltX(const PIXEL_LTRB& src, int x, int y, SURFACE_DDRAW& surf)
{
	RECT rect = { src.left, src.top, src.right, src.bottom };
	DDrawRetryUntilSuccessful(DxObj, [&] {
		return DxObj.Back->BltFast(
			x, y, surf.surf, &rect, DDBLTFAST_SRCCOLORKEY
		);
	});
	return true;
}

void GrpBltXC(const PIXEL_LTRB& src, int x, int y, SURFACE_DDRAW& surf)
{
	RECT rect = { src.left, src.top, src.right, src.bottom };
	DDrawRetryUntilSuccessful(DxObj, [&] {
		return DxObj.Back->BltFast(
			x, y, surf.surf, &rect, DDBLTFAST_NOCOLORKEY
		);
	});
}

// カラーキー＆クリッピング付きＢＭＰ転送 //
// 注意 : src の内容が変更される可能性があります //
bool STD_GrpBlt(PIXEL_LTRB *src, int x, int y, SURFACE_DDRAW& surf)
{
	// 完全に範囲外なら何もしない //
	if(!GrpClip(src, &x, &y)) {
		return false;
	}
	return GrpBltX(*src, x, y, surf);
}

// ビット深度によりカラーキーをセットする
bool GRAPHICS_DDRAW::SurfaceSetColorKeyFromFirstPixel(SURFACE_DDRAW& surf)
{
	DDSURFACEDESC			ddsd;
	DDCOLORKEY				key;

	// Lock する //
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	if(surf.surf->Lock(nullptr, &ddsd, DDLOCK_WAIT, nullptr) != DD_OK) {
		return false;
	}

	// はじっこの色獲得＆カラーキー準備 //
	const auto temp = std::visit([&ddsd](auto pixel) -> DWORD {
		if(sizeof(decltype(pixel)) == 3) {
			// これでいいのか？ //
			const auto *bytes = static_cast<uint8_t *>(ddsd.lpSurface);
			return (
				(Cast::up<DWORD>(bytes[0]) <<  0) +
				(Cast::up<DWORD>(bytes[1]) <<  8) +
				(Cast::up<DWORD>(bytes[2]) << 16)
			);
		}
		return *static_cast<decltype(pixel) *>(ddsd.lpSurface);
	}, PixelFormat);

	// Unlock する //
	const auto dxval = surf.surf->Unlock(nullptr);

	// カラーキーをセットする//
	key.dwColorSpaceHighValue = temp;
	key.dwColorSpaceLowValue  = temp;
	surf.surf->SetColorKey(DDCKEY_SRCBLT, &key);
	if(dxval!=DD_OK) return FALSE;
	else             return TRUE;
}

bool SURFACE_GDI::Load(const BMP_OWNED& bmp)
{
	if(DxObj.PixelFormat.IsPalettized() && !bmp.palette.empty()) {
		// Yup, we must mutate the .BMP palette in addition to the DirectDraw
		// palette, since we pass it via [bmp.info] into CreateDIBSection()
		// below. Not doing that will turn the black parts of Marisa's sprite
		// transparent.
		bmp.palette[0] = { 0, 0, 0 }; // 例外的な処理 //
	}

	Delete();

	void* hbmp_bits = nullptr;
	img = CreateDIBSection(
		dc,
		reinterpret_cast<const BITMAPINFO *>(&bmp.info),
		DIB_RGB_COLORS,
		&hbmp_bits,
		nullptr,
		0
	);
	if(!img) {
		return false;
	}
	memcpy(hbmp_bits, bmp.pixels.data(), bmp.pixels.size_bytes());
	stock_img = SelectObject(dc, img);
	return true;
}

bool GRAPHICS_DDRAW::SurfaceRestore(SURFACE_DDRAW& surf)
{
	if(!surf.surf || !surf.img) {
		return true;
	}
	if(surf.surf->Restore() != DD_OK) {
		return false;
	}

	// Set the palette first to ensure that the colors get mapped correctly
	// when blitting the DIB to the DirectDraw surface.
	if(PixelFormat.IsPalettized()) {
		std::array<RGBQUAD, decltype(Palette)().size()> pal;
		if(GetDIBColorTable(surf.dc, 0, pal.size(), pal.data()) == pal.size()) {
			// Convert BGR to RGB...
			for(size_t i = 0; i < Palette.size(); i++) {
				const auto& bgr = pal[i];
				Palette[i] = { bgr.rgbRed, bgr.rgbGreen, bgr.rgbBlue };
			}

			RELEASE(Pal);
			if(!GrpSetPalette(Palette)) {
				return false;
			}
			surf.surf->SetPalette(Pal);
		}
	}

	// DIB から DC を作成 //
	SurfaceEdit(surf, [](auto, auto) { return true; });

	// カラーキーをセットする //
	return SurfaceSetColorKeyFromFirstPixel(surf);
}

bool GRAPHICS_DDRAW::SurfaceUpdateFromGDI(
	SURFACE_DDRAW& surf, const PIXEL_LTWH& r
)
{
	HDC ddraw_dc;
	if(surf.surf->GetDC(&ddraw_dc) != DD_OK) {
		return false;
	}
	return (
		BitBlt(
			ddraw_dc, r.left, r.top, r.w, r.h, surf.dc, r.left, r.top, SRCCOPY
		) &&
		(surf.surf->ReleaseDC(ddraw_dc) == DD_OK)
	);
}

static bool CreateZbuffer(const XD3DENUM& D3De)
{
	HRESULT				dxret;
	DDSURFACEDESC		ddsd;
	DWORD				mem_flag;

	// Ｚバッファが必要ない場合 //
	if(D3De.Zbuf==0) {
		return true;
	}

	ZeroMemory(&ddsd, sizeof(ddsd));

	mem_flag = (D3De.bIsHW ? DDSCAPS_VIDEOMEMORY : DDSCAPS_SYSTEMMEMORY);
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS|DDSD_ZBUFFERBITDEPTH;
	ddsd.dwWidth = GRP_RES.w;
	ddsd.dwHeight = GRP_RES.h;
	ddsd.dwZBufferBitDepth = D3De.Zbuf;
	ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER|mem_flag;
	dxret = DxObj.DD->CreateSurface(&ddsd,&DxObj.Zbuf,NULL);
	if(dxret!=D3D_OK) return FALSE;

	// Ｚバッファをバックバッファに接続 //
	dxret = DxObj.Back->AddAttachedSurface(DxObj.Zbuf);
	if(dxret!=DD_OK) return FALSE;

	return TRUE;
}

static BOOL FAR __stdcall DriverEnum(GUID FAR *lpGUID,LPSTR lpDDesc,LPSTR lpDName,LPVOID lpContext)
{
	HRESULT			dxret;
	LPDIRECTDRAW	lpDD  = NULL;
	LPDIRECT3D2		lpD3D = NULL;
	DDCAPS			DriverCaps,HelCaps;
	XDDRAWENUM		*xenum = &DxEnum[DxEnumNow];

	// これは作成できないね //
	if(DirectDrawCreate(lpGUID,&lpDD,NULL)!=DD_OK) return DDENUMRET_OK;

	// ドライバの能力取得 //
	ZeroMemory(&DriverCaps, sizeof(DDCAPS)); DriverCaps.dwSize = sizeof(DDCAPS);
	ZeroMemory(&HelCaps, sizeof(DDCAPS));    HelCaps.dwSize    = sizeof(DDCAPS);

	if(lpDD->GetCaps(&DriverCaps,&HelCaps)==DD_OK){
		// ２Ｄでサポートしている解像度をチェック(色数だが...) //
		dxret = lpDD->EnumDisplayModes(0, nullptr, xenum, ModeEnum);

		if(dxret==DD_OK){
			// このドライバで使用できる３Ｄデバイスを列挙         //
			// これが実行できないのはＤＸ３以下であると考えられる //
			// ただし、失敗しても列挙は中断しない                 //
			auto **ptr = reinterpret_cast<void **>(&lpD3D);
			dxret = lpDD->QueryInterface(IID_IDirect3D2, ptr);
			if(dxret==DD_OK)
				lpD3D->EnumDevices(DeviceEnum, xenum);

			// この時点で xenum->D3DNow==0 なら３Ｄは使用できないことになる //
			xenum->name    = lpDName;
			xenum->lpDDGuid = lpGUID;
			DxEnumNow++;
		}
	}

	RELEASE(lpD3D);
	RELEASE(lpDD);

	return DDENUMRET_OK;
}

[[gsl::suppress(con.3)]]
static HRESULT FAR __stdcall DeviceEnum(LPGUID lpGUID,LPSTR lpDeviceDesc,LPSTR lpdeviceName,
						LPD3DDEVICEDESC lpHWDesc,LPD3DDEVICEDESC lpHELDesc,LPVOID lpContext)
{
	BOOL		hw_flag;
	DWORD		zbuf;
	DWORD		depth;
	XD3DENUM	*xd3d;

	// ハードウェアかどうか //
	if(lpHWDesc->dcmColorModel) hw_flag = TRUE;
	else                        hw_flag = FALSE;
	//if(!hw_flag) return D3DENUMRET_OK;

	// ３Ｄカードがサポートするビット深度等のチェックする //
	if(hw_flag){
		depth = lpHWDesc->dwDeviceRenderBitDepth;
		zbuf  = lpHWDesc->dwDeviceZBufferBitDepth;

		// 必要な性能をサポートしているか //
		if(!ConfirmDevice3D(lpHWDesc)) return D3DENUMRET_OK;
	}
	else{
		depth = lpHELDesc->dwDeviceRenderBitDepth;
		zbuf  = lpHELDesc->dwDeviceZBufferBitDepth;

		// 必要な性能をサポートしているか //
		if(!ConfirmDevice3D(lpHELDesc)) return D3DENUMRET_OK;
	}

	// ポインタをセット //
	auto *xddraw = static_cast<XDDRAWENUM *>(lpContext);
	xd3d = (xddraw->D3D) + (xddraw->D3DNow);		// ポインタ＋整数であることに注意

	// どの色深度をサポートしているかな(８ビットは無視する) //
	if((depth & DDBD_16) && xddraw->b16Bit)		xd3d->b16Bit = TRUE;
	if((depth & DDBD_24) && xddraw->b24Bit)		xd3d->b24Bit = TRUE;
	if((depth & DDBD_32) && xddraw->b32Bit)		xd3d->b32Bit = TRUE;

	// Ｚバッファの深さのうちもっとも浅いものを選択する //
	xd3d->Zbuf = 0;							// どのｉｆにもかからないときは０
	if(zbuf & DDBD_32)	xd3d->Zbuf = 32;
	if(zbuf & DDBD_24)	xd3d->Zbuf = 24;
	if(zbuf & DDBD_16)	xd3d->Zbuf = 16;
	//if(zbuf & DDBD_8)	xd3d->Zbuf = 8;

	// ＧＵＩＤ＆デバイス名を保存するぞ //
	xd3d->lpGuid = lpGUID;			// ＧＵＩＤ
	xd3d->name   = lpdeviceName;	// デバイス名
	xd3d->bIsHW  = hw_flag;			// ハードウェアか

	xddraw->D3DNow++;

	return D3DENUMRET_OK;
}

static bool ConfirmDevice3D(const D3DDEVICEDESC *lpDesc)
{
	// 半透明をサポートしているかな？ //
	// DX5 の HEL はサポートしていないので結局ハード側でサポートしていなければ //
	// 実行することはできないのでありました。 //
	// でも、DX7-RGB で動いたとしても速度が1/60ぐらいになるので結局使えないけどね... //

	if((lpDesc->dpcTriCaps.dwDestBlendCaps & D3DBLEND_ONE)==0) return FALSE;

	return TRUE;
}

[[gsl::suppress(con.3)]]
static HRESULT FAR __stdcall ModeEnum(DDSURFACEDESC *pddsd,VOID *pInfo)
{
	auto *xenum = static_cast<XDDRAWENUM *>(pInfo);
	int				color;

	// ６４０Ｘ４８０についてだけ調べるようにする //
	if((pddsd->dwWidth != GRP_RES.w) || (pddsd->dwHeight != GRP_RES.h)) {
		return DDENUMRET_OK;
	}

	// どの色か //
	color  = pddsd->ddpfPixelFormat.dwRGBBitCount;
	switch(color){
		case(8):	xenum->b8Bit  = TRUE;	break;
		case(16):	xenum->b16Bit = TRUE;	break;
		case(24):	xenum->b24Bit = TRUE;	break;
		case(32):	xenum->b32Bit = TRUE;	break;
	}

	return DDENUMRET_OK;
}

static bool DM_GRPBLT(PIXEL_LTRB *src, int x, int y, SURFACE_DDRAW& surf)
{
	return false;
}
