/*                                                                           */
/*   DD_UTY.cpp   描画関連の関数                                             */
/*                                                                           */
/*                                                                           */

#include "DD_UTY.H"
#include "DD_GRP2D.H"					// ２Ｄ用描画関数(公開しないこと)
#include "DD_GRP3D.H"					// ３Ｄ用描画関数(公開しないこと)
#include <d3d.h>
#include <stdio.h>
#pragma message(PBGWIN_DD_UTY_H)


// 図形描画関数に対するダミー関数 //
static void DM_V(void);
static void DM_BBBB(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
static void DM_BBB(uint8_t a, uint8_t b, uint8_t c);
static void DM_BB(uint8_t a, uint8_t b);
static void DM_III(int a, int b, int c);
static void DM_IIII(int a, int b, int c, int d);
static void DM_PI(POINT *p, int n);
static bool DM_RESTORE(void);
static void DM_GRPBLT(RECT *src, int x, int y, SURFACE_DDRAW* surf);

// クリッピング枠 //
int CLIP_X_MIN = 0;
int CLIP_Y_MIN = 0;
int CLIP_X_MAX = 639;
int CLIP_Y_MAX = 479;

// キャプチャーを行うか(->MtoB) //
BOOL CaptureFlag = FALSE;
void STD_GrpBlt(RECT *src, int x, int y, SURFACE_DDRAW* surf);


// Ｚバッファを必要なら生成し、接続する //
static bool CreateZbuffer(const XD3DENUM& D3De);

// 列挙用コールバック関数群 //
static BOOL FAR __stdcall DriverEnum(GUID FAR *lpGUID,LPSTR lpDDesc,LPSTR lpDName,LPVOID lpContext);
static HRESULT FAR __stdcall DeviceEnum(LPGUID lpGUID,LPSTR lpDeviceDesc,LPSTR lpdeviceName,LPD3DDEVICEDESC lpHWDesc,LPD3DDEVICEDESC lpHELDesc,LPVOID lpContext);
static HRESULT FAR __stdcall ModeEnum(DDSURFACEDESC *pddsd,VOID *pInfo);

// デバイスが求める性能を持っているか //
static BOOL ConfirmDevice3D(LPD3DDEVICEDESC lpDesc);

// ダミー関数をセットする //
static void SetDummyFunc(void);


// フリップ用関数 //
static void GrpFlipFS(void);
static void GrpFlipWnd(void);

// ビット深度によりカラーキーをセットする
extern BOOL GrpSetColorKey(LPDIRECTDRAWSURFACE lpds);


// グローバル変数 //
GRAPHICS_DDRAW	DxObj;
SURFACE_DDRAW	DxSurf[SURFACE_MAX];
XDDRAWENUM		DxEnum[XDDRAWENUM_MAX];
DWORD			DxEnumNow;

// 図形描画用関数ポインタの実体 //
void(*GrpFlip)(void)                                = DM_V;
void(*GrpLock)(void)                                = DM_V;
void(*GrpUnlock)(void)                              = DM_V;
void(*GrpSetColor)(uint8_t r, uint8_t g, uint8_t b) = DM_BBB;
void(*GrpSetAlpha)(uint8_t a, uint8_t mode)         = DM_BB;
void(*GrpLine)(int x1, int y1, int x2, int y2)      = DM_IIII;
void(*GrpBox)(int x1, int y1, int x2, int y2)       = DM_IIII;
void(*GrpCircle)(int x, int y, int r)               = DM_III;
void(*GrpCircleF)(int x, int y, int r)              = DM_III;
void(*GrpPolygon)(POINT *p, int n)                  = DM_PI;
void(*GrpBoxA)(int x1, int y1, int x2, int y2)      = DM_IIII;
void(*GrpCircleA)(int x, int y, int r)              = DM_III;
void(*GrpPolygonA)(POINT *p, int n)                 = DM_PI;
void(*GrpFatCircleA)(int x, int y, int r, int w)    = DM_IIII;
void(*GrpBlt)(RECT *src, int x, int y, SURFACE_DDRAW* surf) = STD_GrpBlt;

static bool(*RestoreHookFn)(void) = DM_RESTORE;	// Restore の事後処理用関数

// Templating the functor avoids both the need for #including <functional> and
// any potential runtime exceptions related to the call.
void DDrawRetryUntilSuccessful(GRAPHICS_DDRAW& dxobj, auto action)
{
	for(;;) {
		HRESULT ddrval = action();
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


bool GrpEnum(HWND hWnd)
{
	HRESULT		dxret;

	ZEROMEM(DxEnum);
	DxEnumNow = 0;

	try{
		// 列挙開始 //
		dxret = DirectDrawEnumerate(DriverEnum,NULL);
		if(dxret!=DD_OK) throw("DirectDrawEnumerate に失敗");

		// 一応ダミー関数をセットしておく //
		SetDummyFunc();
	}
	catch(char *error){
		DebugOut(error);
		return FALSE;
	}

	DxObj.hWnd = hWnd;

#ifdef _DEBUG
	DebugOut("正常に列挙できたの");
#endif

	return TRUE;
}

bool GRAPHICS_DDRAW::Init(uint8_t& device_id, BITDEPTH& bitdepth)
{
	HRESULT				dxret;
	LPDIRECTDRAW		lpDD  = NULL;
	DDSURFACEDESC		ddsd;
	DDSCAPS				ddscaps;
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
	ZEROMEM(ddsd);
	ZEROMEM(ddscaps);

	try{
		dxret = DirectDrawCreate(pXDD.lpDDGuid, &lpDD, nullptr);
		if(dxret!=DD_OK) throw("DirectDrawCreate に失敗");

		dxret = lpDD->QueryInterface(IID_IDirectDraw2, (void **)&DD);
		if(dxret!=DD_OK) throw("QueryInterface に失敗");
		RELEASE(lpDD);

		// 強調レベルのセット //
		dxret = DD->SetCooperativeLevel(
			hWnd, (DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN)
		);
		if(dxret!=DD_OK) throw("SetCooperativeLevel に失敗");

		// ディスプレイモードのセット //
		dxret = DD->SetDisplayMode(640, 480, bitdepth.value(), 0, 0);
		if(dxret!=DD_OK) throw("SetDisplayMode に失敗");

		// プライマリ＆バックサーフェスの作成 //
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS|DDSD_BACKBUFFERCOUNT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE|DDSCAPS_3DDEVICE|DDSCAPS_FLIP|DDSCAPS_COMPLEX;
		ddsd.dwBackBufferCount = 1;
		dxret = DD->CreateSurface(&ddsd, &Prim, nullptr);
		if(dxret!=DD_OK) throw("CreateSurface(Prim) に失敗");

		// BackBuffer へのポインタを Get //
		ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
		dxret = Prim->GetAttachedSurface(&ddscaps, &Back);
		if(dxret!=DD_OK) throw("GetAttachedSurface に失敗");

		// フルスクリーン用フリップ //
		GrpFlip = GrpFlipFS;

		// 2Dの初期化はここでおしまい //
		DeviceID = device_id;
		PixelFormat = pixel_format;
		if(PixelFormat.IsPalettized()) throw(TRUE);

		// Direct3D Object の取得 //
		dxret = DD->QueryInterface(IID_IDirect3D2, (void **)&D3);
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

		throw(TRUE);
	}
	catch(char *error){
		DebugOut(error);
		Cleanup();
		return FALSE;
	}
	catch(BOOL flag){
		if(pixel_format.IsPalettized()) {
			_2DSetup();
		} else {
			_3DSetup();
		}

		GrpSetClip(0,0,640,480);
		return flag;
	}

	return FALSE;
}

void GRAPHICS_DDRAW::Cleanup(void)
{
	int		i;

	RELEASE(View);
	RELEASE(D3Dev);
	RELEASE(D3);
	RELEASE(Zbuf);

	RELEASE(Pal);

	// ここではSurfaceのBMPを解放しないことに注意 //
	for(i=0;i<SURFACE_MAX;i++){
		RELEASE(DxSurf[i].surf);
	}

	RELEASE(Back);
	RELEASE(Prim);
	RELEASE(DD);

	// グラフィック系関数を実行できないようにする //
	SetDummyFunc();
}

// クリッピング枠を取得する //
void GrpGetClip(RECT *dest)
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
	CLIP_X_MAX = (ex>639) ? 639 : ex;
	CLIP_Y_MAX = (ey>479) ? 479 : ey;

	if ((CLIP_X_MIN == CLIP_X_MAX) || (CLIP_Y_MIN == CLIP_Y_MAX)) {
		GrpBlt = DM_GRPBLT;
	} else {
		GrpBlt = STD_GrpBlt;
	}

	if(DxObj.View){
		view.dwSize = sizeof(view);
		view.dwX          = sx;
		view.dwY          = sy;
		view.dwWidth      = ex-sx;
		view.dwHeight     = ey-sy;
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
	HRESULT		dxval;
	int			i;

	if(Prim) {
		dxval = Prim->Restore();
		if(dxval!=DD_OK) return FALSE;
	}
	if(Back) {
		Back->Restore();
		if(dxval!=DD_OK) return FALSE;
	}
	if(Zbuf) {
		Zbuf->Restore();
		if(dxval!=DD_OK) return FALSE;
	}

	for(i=0;i<SURFACE_MAX;i++){
		if(!SurfaceRestore(DxSurf[i])) {
			return false;
		}
	}

	return RestoreHookFn();
}

// Restore の事後処理フック //
void GrpSetRestoreHook(bool(*func)(void))
{
	// 安全対策(上を見れば分かるように、NULLチェックがないので...)
	if(func==NULL) RestoreHookFn = DM_RESTORE;
	else           RestoreHookFn = func;
}

std::span<PALETTEENTRY> GRAPHICS_DDRAW::PaletteForDDraw()
{
	// Verify that PALETTEENTRY and RGBA are identical.
	using COLOR = decltype(Palette)::value_type;
	static_assert(sizeof(COLOR) == sizeof(PALETTEENTRY));
	static_assert(offsetof(COLOR, r) == offsetof(PALETTEENTRY, peRed));
	static_assert(offsetof(COLOR, g) == offsetof(PALETTEENTRY, peGreen));
	static_assert(offsetof(COLOR, b) == offsetof(PALETTEENTRY, peBlue));
	static_assert(offsetof(COLOR, a) == offsetof(PALETTEENTRY, peFlags));
	return { reinterpret_cast<PALETTEENTRY *>(Palette.data()), Palette.size() };
}

// 現在のパレットを取得する(ただの代入なのでこの関数を使うまでも無いかもしれないね) //
void GrpGetPalette(PALETTE& pal)
{
	// ８Ｂｉｔモード以外では、何もしないでリターンする //
	if(DxObj.PixelFormat.IsChanneled()) return;

	pal = DxObj.Palette;
}

void GrpSetPalette(const PALETTE& pal)
{
	// ８Ｂｉｔモード以外では、何もしないでリターンする //
	if(DxObj.PixelFormat.IsChanneled()) return;

	DxObj.Palette = pal;
	DxObj.Palette[0] = { 0, 0, 0 };	// 強制的に色をセットしてしまう //

	// パレットを変更
	auto p = DxObj.PaletteForDDraw();
	DxObj.Pal->SetEntries(0, 0, p.size(), p.data());
	DxObj.bNeedChgPal = TRUE;

	DisableAlphaTable();
}

SURFACE_DDRAW* GRAPHICS_DDRAW::SurfaceCreate(BYTE surf_id, PIXEL_SIZE size)
{
	DDSURFACEDESC		ddsd;
	IDirectDrawSurface*	temp = nullptr;


	if(surf_id>=SURFACE_MAX) return NULL;
	RELEASE(DxSurf[surf_id].surf);
	DxSurf[surf_id].size = { 0, 0 };

	ZEROMEM(ddsd);

	ddsd.dwSize         = sizeof(ddsd);
	ddsd.dwWidth        = size.w;
	ddsd.dwHeight       = size.h;
	ddsd.dwFlags        = DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	auto dxval = DD->CreateSurface(&ddsd, &temp, nullptr);

	if(dxval==DD_OK){
		DxSurf[surf_id].surf = temp;
		DxSurf[surf_id].size = size;
		SurfaceRestore(DxSurf[surf_id]);
	}

	return &DxSurf[surf_id];
}

bool GRAPHICS_DDRAW::SurfaceLoad(BYTE surf_id, BMP_OWNED bmp)
{
	if(surf_id>=SURFACE_MAX) return FALSE;

	auto& surf = DxSurf[surf_id];
	return (surf.Load(std::move(bmp)) && SurfaceRestore(surf));
}

void GrpCls(void)
{
	return GrpClsEx(
		std::nullopt, ((DxObj.PixelFormat.IsPalettized()) ? RGB256(0, 0, 0) : 0)
	);
}


// 直接番号指定で矩形内を塗りつぶす //
void GrpClsEx(const std::optional<RECT>& Target, DWORD col)
{
	DDBLTFX		ddbltfx;
	RECT	rect_local;
	RECT*	rect = nullptr;

	ZeroMemory(&ddbltfx,sizeof(ddbltfx));
	ddbltfx.dwSize      = sizeof(ddbltfx);
	ddbltfx.dwFillColor = col;

	if(Target) {
		rect_local = Target.value();
		rect = &rect_local;
	}

	DDrawRetryUntilSuccessful(DxObj, [&] {
		return DxObj.Back->Blt(
			rect, nullptr, nullptr, DDBLT_COLORFILL, &ddbltfx
		);
	});
}


extern void GrpOffsClear(LPDIRECTDRAWSURFACE lpds)
{
	DDBLTFX		ddbltfx;

	ZeroMemory(&ddbltfx,sizeof(ddbltfx));
	ddbltfx.dwSize      = sizeof(ddbltfx);
	ddbltfx.dwFillColor = (DxObj.PixelFormat.IsPalettized()) ? RGB256(0,0,0) : 0;
	DDrawRetryUntilSuccessful(DxObj, [&] {
		return lpds->Blt(nullptr, nullptr, nullptr, DDBLT_COLORFILL, &ddbltfx);
	});
	DDCOLORKEY key = { ddbltfx.dwFillColor, ddbltfx.dwFillColor };
	lpds->SetColorKey(DDCKEY_SRCBLT, &key);
}

static void GrpFlipFS(void)
{
	// キャプチャーフラグが有効かつ、キャプチャー要求が出ていれば画像を取り込む //
	if(CaptureFlag && (GetAsyncKeyState(VK_SNAPSHOT)&0x8000))
		PutSurfaceToBmp(DxObj.Back);

	// パレットを変更する必要があれば、変更だ //
	if(DxObj.bNeedChgPal && DxObj.PixelFormat.IsPalettized()){
		DxObj.Back->SetPalette(DxObj.Pal);		// 裏画面にパレットをセットする
		DxObj.bNeedChgPal = FALSE;
	}
	DDrawRetryUntilSuccessful(DxObj, [&] {
		return DxObj.Prim->Flip(nullptr, 0);
	});
}

static void GrpFlipWnd(void)
{
	// キャプチャーフラグが有効かつ、キャプチャー要求が出ていれば画像を取り込む //
	if(CaptureFlag && (GetAsyncKeyState(VK_SNAPSHOT)&0x8000))
		PutSurfaceToBmp(DxObj.Back);

	DDrawRetryUntilSuccessful(DxObj, [&] {
		return DxObj.Prim->Blt(nullptr, DxObj.Back, nullptr, 0, nullptr);
	});
}

bool GrpClip(RECT *src, int *x, int *y)
{
	int w = src->right - src->left - 1;
	int h = src->bottom - src->top - 1;

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

void GrpBltX(RECT *src, int x, int y, SURFACE_DDRAW* surf)
{
	DDrawRetryUntilSuccessful(DxObj, [&] {
		return DxObj.Back->BltFast(
			x, y, surf->surf, src, DDBLTFAST_SRCCOLORKEY
		);
	});
}

void GrpBltXC(RECT *src, int x, int y, SURFACE_DDRAW* surf)
{
	DDrawRetryUntilSuccessful(DxObj, [&] {
		return DxObj.Back->BltFast(x, y, surf->surf, src, DDBLTFAST_NOCOLORKEY);
	});
}

// カラーキー＆クリッピング付きＢＭＰ転送 //
// 注意 : src の内容が変更される可能性があります //
void STD_GrpBlt(RECT *src, int x, int y, SURFACE_DDRAW* surf)
{
	// 完全に範囲外なら何もしない //
	if(!GrpClip(src,&x,&y)) return;
	GrpBltX(src, x, y, surf);
}
extern BOOL GrpSetColorKey(LPDIRECTDRAWSURFACE lpds)
{
	DDSURFACEDESC			ddsd;
	HRESULT					dxval;
	DDCOLORKEY				key;

	// Lock する //
	ZEROMEM(ddsd);ddsd.dwSize = sizeof(ddsd);
	if(lpds->Lock(NULL,&ddsd,DDLOCK_WAIT,NULL)!=DD_OK) return FALSE;

	// はじっこの色獲得＆カラーキー準備 //
	DWORD temp = std::visit([&ddsd](auto pixel) {
		if(sizeof(decltype(pixel)) == 3) {
			// これでいいのか？ //
			auto* bytes = reinterpret_cast<uint8_t *>(ddsd.lpSurface);
			return (
				bytes[0] + (DWORD{ bytes[1] } << 8) + (DWORD{ bytes[2] } << 16)
			);
		}
		return DWORD{ *reinterpret_cast<decltype(pixel) *>(ddsd.lpSurface) };
	}, DxObj.PixelFormat);

	// Unlock する //
	dxval = lpds->Unlock(NULL);

	// カラーキーをセットする//
	key.dwColorSpaceHighValue = temp;
	key.dwColorSpaceLowValue  = temp;
	lpds->SetColorKey(DDCKEY_SRCBLT,&key);
	if(dxval!=DD_OK) return FALSE;
	else             return TRUE;
}

bool SURFACE_GDI::Load(BMP_OWNED bmp)
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
	HRESULT ddrval;

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

			if(Pal) {
				Pal->Release();
			}

			ddrval = DD->CreatePalette(
				(DDPCAPS_8BIT | DDPCAPS_ALLOW256),
				PaletteForDDraw().data(),
				&Pal,
				nullptr
			);
			if(ddrval != DD_OK) {
				return false;
			}
			Prim->SetPalette(Pal);
			Back->SetPalette(Pal);
			surf.surf->SetPalette(Pal);
		}
	}

	// DIB から DC を作成 //
	HDC ddraw_dc;
	ddrval = surf.surf->GetDC(&ddraw_dc);
	if(ddrval!=DD_OK) return FALSE;

	BitBlt(ddraw_dc, 0, 0, surf.size.w, surf.size.h, surf.dc, 0, 0, SRCCOPY);

	// 後始末 //
	surf.surf->ReleaseDC(ddraw_dc);

	// カラーキーをセットする //
	GrpSetColorKey(surf.surf);

	DisableAlphaTable();

	return TRUE;
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

	ZEROMEM(ddsd);

	mem_flag = (D3De.bIsHW ? DDSCAPS_VIDEOMEMORY : DDSCAPS_SYSTEMMEMORY);
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS|DDSD_ZBUFFERBITDEPTH;
	ddsd.dwWidth = 640;
	ddsd.dwHeight = 480;
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
	ZEROMEM(DriverCaps);	DriverCaps.dwSize = sizeof(DDCAPS);
	ZEROMEM(HelCaps);		HelCaps.dwSize    = sizeof(DDCAPS);

	if(lpDD->GetCaps(&DriverCaps,&HelCaps)==DD_OK){
		// ２Ｄでサポートしている解像度をチェック(色数だが...) //
		dxret = lpDD->EnumDisplayModes(0,NULL,(LPVOID)xenum,ModeEnum);

		if(dxret==DD_OK){
			// このドライバで使用できる３Ｄデバイスを列挙         //
			// これが実行できないのはＤＸ３以下であると考えられる //
			// ただし、失敗しても列挙は中断しない                 //
			dxret = lpDD->QueryInterface(IID_IDirect3D2,(LPVOID *)&lpD3D);
			if(dxret==DD_OK)
				lpD3D->EnumDevices(DeviceEnum,(LPVOID)xenum);

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

static HRESULT FAR __stdcall DeviceEnum(LPGUID lpGUID,LPSTR lpDeviceDesc,LPSTR lpdeviceName,
						LPD3DDEVICEDESC lpHWDesc,LPD3DDEVICEDESC lpHELDesc,LPVOID lpContext)
{
	BOOL		hw_flag;
	DWORD		zbuf;
	DWORD		depth;
	XD3DENUM	*xd3d;
	XDDRAWENUM	*xddraw;

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
	xddraw = (XDDRAWENUM *)lpContext;
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

static BOOL ConfirmDevice3D(LPD3DDEVICEDESC lpDesc)
{
	// 半透明をサポートしているかな？ //
	// DX5 の HEL はサポートしていないので結局ハード側でサポートしていなければ //
	// 実行することはできないのでありました。 //
	// でも、DX7-RGB で動いたとしても速度が1/60ぐらいになるので結局使えないけどね... //

	if((lpDesc->dpcTriCaps.dwDestBlendCaps & D3DBLEND_ONE)==0) return FALSE;

	return TRUE;
}

static HRESULT FAR __stdcall ModeEnum(DDSURFACEDESC *pddsd,VOID *pInfo)
{
	XDDRAWENUM		*xenum = (XDDRAWENUM *)pInfo;
	int				color;

	// ６４０Ｘ４８０についてだけ調べるようにする //
	if(pddsd->dwWidth!=640)		return DDENUMRET_OK;
	if(pddsd->dwHeight!=480)	return DDENUMRET_OK;

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

static void SetDummyFunc(void)
{
	GrpFlip     = DM_V;
	GrpLock     = DM_V;
	GrpUnlock   = DM_V;
	GrpSetColor = DM_BBB;
	GrpSetAlpha = DM_BB;
	GrpLine     = DM_IIII;
	GrpBox      = DM_IIII;
	GrpCircle   = DM_III;
	GrpCircleF  = DM_III;
	GrpPolygon  = DM_PI;
	GrpBoxA     = DM_IIII;
	GrpCircleA  = DM_III;
	GrpPolygonA = DM_PI;

	GrpFatCircleA = DM_IIII;
}

// ダミー関数の実体(かなり手抜き...) //
static void DM_V(void) {}
static void DM_BBBB(uint8_t a, uint8_t b, uint8_t c, uint8_t d){};
static void DM_BBB(uint8_t a, uint8_t b, uint8_t c) {}
static void DM_BB(uint8_t a, uint8_t b) {}
static void DM_III(int a, int b, int c) {}
static void DM_IIII(int a, int b, int c, int d) {}
static void DM_PI(POINT *p, int n) {}

static bool DM_RESTORE(void)
{
	// ダミー関数は確実に成功する //
	return TRUE;
}

static void DM_GRPBLT(RECT *src, int x, int y, SURFACE_DDRAW* surf) {}
