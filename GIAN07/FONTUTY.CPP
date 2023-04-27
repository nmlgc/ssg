/*                                                                           */
/*   FontUty.cpp   フォントの処理いろいろ                                    */
/*                                                                           */
/*                                                                           */

#include "FONTUTY.H"
#include "LOADER.H"
#include "WindowSys.h"
#include "game/coords.h"
#include "DirectXUTYs/DD_UTY.H"
#include "DirectXUTYs/UT_MATH.H"
#include <ddraw.h>

// ヒミツの関数 //
void __FillExpoint(EXPOINT *p, int x, int y, int w, int h);

// Glyph selection inside the 16×16 font //
std::optional<PIXEL_LTRB> Glyph16x16(char c)
{
	PIXEL_LTWH src;
	src.w = 16;
	src.h = 16;

	// GrTama における文字の並びは次のようになっている。 //
	// (変更する可能性もあるので十分に注意すること)      //
	// ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789              //
	// abcdefghijklmnopqrstuvwxyz!?#\<>=,+-              //

	if((c >= 'A') && (c <= 'Z')) {
		src.left = ((c - 'A') << 4); src.top = (480 - 32);
	} else if((c >= 'a') && (c <= 'z')) {
		src.left = ((c - 'a') << 4); src.top = (480 - 16);
	} else if((c >= '0') && (c <= '9')) {
		src.left = (((c - '0') << 4) + 416); src.top = (480 - 32);
	} else {
		switch(c) {
		case('!'):	src.left = ((0 << 4) + 416); src.top = (480 - 16);	break;
		case('?'):	src.left = ((1 << 4) + 416); src.top = (480 - 16);	break;
		case('#'):	src.left = ((2 << 4) + 416); src.top = (480 - 16);	break;
		case('\\'):	src.left = ((3 << 4) + 416); src.top = (480 - 16);	break;
		case('<'):	src.left = ((4 << 4) + 416); src.top = (480 - 16);	break;
		case('>'):	src.left = ((5 << 4) + 416); src.top = (480 - 16);	break;
		case('='):	src.left = ((6 << 4) + 416); src.top = (480 - 16);	break;
		case(','):	src.left = ((7 << 4) + 416); src.top = (480 - 16);	break;
		case('+'):	src.left = ((8 << 4) + 416); src.top = (480 - 16);	break;
		case('-'):	src.left = ((9 << 4) + 416); src.top = (480 - 16);	break;
		default:	return std::nullopt;
		}
	}
	return src;
}


// 16x16 透過フォントで文字列出力(高速) //
extern void GrpPut16(int x, int y, const char *s)
{
	int		sx,tx,ty;

	sx = x;

	for(;(*s)!='\0';s++,x+=14){
		auto maybe_src = Glyph16x16(*s);
		if(maybe_src) {
			auto& src = maybe_src.value();

			tx = x;ty = y;
			if(tx>=0 && tx<630)	// 安全対策???
				GrpBltX(&src,tx,ty,GrTama);
		}
	}
}

// 16x16 透過フォントで文字列出力(高速) //
extern void GrpPut16c(int x, int y, char *s)
{
	int		sx,tx,ty;

	sx = x;

	for(;(*s)!='\0';s++,x+=14){
		auto maybe_src = Glyph16x16(*s);
		if(maybe_src) {
			auto& src = maybe_src.value();

			tx = x;ty = y;
			if(tx>=0 && tx<630)	// 安全対策???
				GrpBlt(&src,tx,ty,GrTama);
		}
	}
}

// 上と同じだが、ｘ移動幅が１６ //
extern void GrpPut16c2(int x, int y, char *s)
{
	int		sx,tx,ty;

	sx = x;

	for(;(*s)!='\0';s++,x+=16){
		auto maybe_src = Glyph16x16(*s);
		if(maybe_src) {
			auto& src = maybe_src.value();

			tx = x;ty = y;
			//if(tx>=0 && tx<630)	// 安全対策???
			GrpBlt(&src,tx,ty,GrTama);
		}
	}
}

// 16x16 透過フォントで文字出力(クリッピング有) //
extern void GrpPutc(int x, int y, char c)
{
	auto maybe_src = Glyph16x16(c);
	if(maybe_src) {
		auto& src = maybe_src.value();

		GrpBlt(&src, x, y, GrTama);
	}
}

// 05x07 べた貼りフォント //
extern void GrpPut57(int x, int y, char *s)
{
	PIXEL_LTRB	src;
	int		sx,tx,ty;

	sx = x;

	for(;(*s)!='\0';s++,x+=6){
		if((*s)>='0' && (*s)<='9'){
			src = PIXEL_LTWH{ (((*s - '0') << 3) + 128), 80, 5, 7 };
		}
		else{
			continue;
		}

		tx = x;ty = y;
		if(tx>=0 && tx<630)	// 安全対策???
			GrpBlt(&src,tx,ty,GrTama);
	}
}

// 07x11 音楽室用フォント
extern void GrpPut7B(int x, int y, char *s)
{
	PIXEL_LTRB	src;
	for(;(*s)!='\0';s++,x+=8){
		if((*s)>='0' && (*s)<='9'){
			src = PIXEL_LTWH{ (((*s - '0') << 3) + 184), 456, 7, 11 };
		}
		else if((*s)=='-'){
			src = PIXEL_LTWH{ (264 + 0), 456, 7, 11 };
		}
		else if((*s)==':'){
			src = PIXEL_LTWH{ (264 + 8), 456, 7, 11 };
		}
		else{
			continue;
		}

		if((x >= 0) && (x < 630)) { // 安全対策???
			GrpBltX(&src, x, y, GrMusic);
		}
	}
}

// 得点アイテムのスコアを描画 //
extern void GrpPutScore(int x, int y, char *s)
{
	PIXEL_LTRB	src;
	int		sx,tx,ty;

	sx = x;

	for(;(*s)!='\0';s++,x+=6){
		if((*s)>='0' && (*s)<='9'){
			src = PIXEL_LTWH{ (((*s - '0') << 3) + 128), 88, 5, 7 };
		}
		else{
			continue;
		}

		tx = x;ty = y;
		if(tx>=0 && tx<630)	// 安全対策???
			GrpBlt(&src,tx,ty,GrTama);
	}
}

// MIDI 用フォントを描画する //
extern void GrpPutMidNum(int x, int y, int n)
{
	char	buf[10];
	int		i;
	PIXEL_LTRB	src;

	sprintf(buf,"%3d",n);

	// n = 1Byte ならば３桁以内に収まるハズ //
	for(i=0;i<3;i++,x+=5){
		if(buf[i]>='0' && buf[i]<='9'){
			src = PIXEL_LTWH{ (80 + ((buf[i] - '0') * 4)), 432, 4, 5 };
			GrpBlt(&src, x, y, GrMusic);
		}
		if(buf[i]=='-'){
			src = PIXEL_LTWH{ (80 + (10 * 4)), 432, 4, 5 };
			GrpBlt(&src, x, y, GrMusic);
		}
	}
}

// フォントをセットする //
extern void InitFonts(void)
{
	// でかいフォント //
	WinGrpInfo.LargeFont = CreateFont(24,12,0,0,500,FALSE,0,0,SHIFTJIS_CHARSET,
		OUT_TT_ONLY_PRECIS,CLIP_DEFAULT_PRECIS,PROOF_QUALITY,FIXED_PITCH,"ＭＳ ゴシック");

	// 並なフォント //
	WinGrpInfo.NormalFont = CreateFont(16,8,0,0,400,FALSE,0,0,SHIFTJIS_CHARSET,
		OUT_TT_ONLY_PRECIS,CLIP_DEFAULT_PRECIS,PROOF_QUALITY,FIXED_PITCH,"ＭＳ ゴシック");

	// ちっこいフォント //
	WinGrpInfo.SmallFont = CreateFont(14,7,0,0,400,FALSE,0,0,SHIFTJIS_CHARSET,
		OUT_TT_ONLY_PRECIS,CLIP_DEFAULT_PRECIS,PROOF_QUALITY,FIXED_PITCH,"ＭＳ ゴシック");
}

// フォントを解放する //
extern void CleanupFont(void)
{
	if(WinGrpInfo.LargeFont)	DeleteObject(WinGrpInfo.LargeFont);
	if(WinGrpInfo.NormalFont)	DeleteObject(WinGrpInfo.NormalFont);
	if(WinGrpInfo.SmallFont)	DeleteObject(WinGrpInfo.SmallFont);
}

void DrawGrdFont(SURFACE_DDRAW& surf, std::string_view str)
{
	HDC			hdc;
	HFONT		oldfont;
	int			j;
	DWORD		temp,old;

	// The MIDI sequence titles passed to this function tend to come with lots
	// of trailing whitespace. Adding 1 also turns `npos` to 0.
	str = str.substr(0, (str.find_last_not_of(" ") + 1));

	DDCOLORKEY key = { RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00) };
	surf.surf->SetColorKey(DDCKEY_SRCBLT, &key);

	surf.surf->GetDC(&hdc);

	// Since we render the string multiple times on top of each other, we can't
	// rely on SetBkMode(hdc, OPAQUE) for clearing the bitmap. The text might
	// also be smaller than the one we rendered previously.
	const RECT full = { 0, 0, surf.size.w, surf.size.h };
	FillRect(hdc, &full, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

	// ここら辺は、一種の常套手段か？ //
	old = GetPixel(hdc,0,0);
	SetPixel(hdc,0,0,RGB(255,255,255));
	temp = GetPixel(hdc,0,0);
	SetPixel(hdc,0,0,old);

	oldfont = (HFONT)SelectObject(hdc,WinGrpInfo.NormalFont);
	SetBkMode(hdc,TRANSPARENT);

	SetTextColor(hdc, RGB(0, 0, 128));
	TextOut(hdc, 2, 2, str.data(), str.size());
	TextOut(hdc, 1, 2, str.data(), str.size());
	SetTextColor(hdc, RGB(255, 255, 255));
	TextOut(hdc, 1, 1, str.data(), str.size());
	TextOut(hdc, 0, 1, str.data(), str.size());

#define FSIZE 8
	//COLORREF temp = GetPixel(hdc,0,0);
	const PIXEL_COORD w = (str.size() * 16);
	for(PIXEL_COORD i = 1; i < w; i++) {
		for(auto j = 1; j < 16; j++) {
			if(GetPixel(hdc,i,j)==temp)//RGB(255,255,255))
				SetPixel(hdc,i,j,RGB(255+FSIZE-j*FSIZE,255+FSIZE-j*FSIZE,255));
		}
	}

	SelectObject(hdc,oldfont);
	surf.surf->ReleaseDC(hdc);
}


// フォント生成 //
ExtraFontInfo *CreateExtraFont(SURFACE_DDRAW& Surface, PIXEL_LTRB *pSrc)
{
	ExtraFontInfo		*pInfo;
	int					MaxSize;
	DWORD				Width, Height;
	void				*Target;

	DDSURFACEDESC		ddsd;

	// 俺様メモリの確保ぉ //
	pInfo = (ExtraFontInfo *)LocalAlloc(LPTR, sizeof(ExtraFontInfo));
	if(pInfo == NULL) return NULL;

	// 幅と高さを代入しましょう //
	pInfo->Width  = Width  = (pSrc->right)  - (pSrc->left);
	pInfo->Height = Height = (pSrc->bottom) - (pSrc->top);

	// 画像格納に必要なサイズを求める //
	MaxSize = pInfo->Width * pInfo->Height * sizeof(EXPOINT);

	pInfo->Data = (EXPOINT *)LocalAlloc(LPTR, MaxSize);
	if(pInfo->Data == NULL){
		LocalFree(pInfo);
		return NULL;
	}

	memset(&ddsd,0,sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(ddsd);
	auto ddret = Surface.surf->Lock(nullptr, &ddsd, DDLOCK_WAIT, nullptr);
	if(ddret != DD_OK){
		DeleteExtraFont(pInfo);
		return NULL;
	}

	pInfo->DataSize = 0;

	// 透過色をセットしましょう //
	std::visit([&ddsd, &pInfo, &pSrc, &Width, &Height](auto pixel) {
		auto* pPixel = reinterpret_cast<decltype(pixel) *>(ddsd.lpSurface);
		const auto TransID = pPixel[0];

		for(decltype(Height) y = 0; y < Height; y++) {
			for(decltype(Width) x = 0; x < Width; x++) {
				const auto offset = (
					(x+pSrc->left) + ((y+pSrc->top) * ddsd.dwWidth)
				);
				if(pPixel[offset] != TransID){
					pInfo->Data[pInfo->DataSize].c = pPixel[offset];
					__FillExpoint(&(pInfo->Data[pInfo->DataSize]), x, y, Width, Height);
					pInfo->DataSize++;
				}
			}
		}
	}, DxObj.PixelFormat);

	Surface.surf->Unlock(nullptr);

	return pInfo;
}


void __FillExpoint(EXPOINT *p, int x, int y, int w, int h)
{
	x -= w / 2;
	y -= h / 2;

	p->x = x;
	p->y = y;

	p->d = atan8(x * 64, y * 64);
	p->l = isqrt(x * x + y * y);
}


// フォント削除 //
void DeleteExtraFont(ExtraFontInfo *pFont)
{
	if(pFont == NULL) return;

	if(pFont->Data) LocalFree(pFont->Data);

	LocalFree(pFont);
}


// フォント描画 //
void DrawExtraFont(ExtraFontInfo *pFont, int ox, int oy, int t, int radius_max)
{
	DDSURFACEDESC		ddsd;

	if(pFont == NULL) return;


	memset(&ddsd,0,sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(ddsd);
	const auto ddret = DxObj.Back->Lock(NULL,&ddsd,DDLOCK_WAIT,NULL);
	if(ddret != DD_OK) return;

	std::visit([&pFont, &ox, &oy, &t, &radius_max, &ddsd](auto pixel) {
		auto* pData = pFont->Data;
		auto* pPixel = reinterpret_cast<decltype(pixel) *>(ddsd.lpSurface);

		for(decltype(pFont->DataSize) i = 0; i < pFont->DataSize; i++, pData++){
			int x, y;
			if(t){
				//d  = pData->d + (pData->l * (sinl(t-64,128)+128)) / 64;
				const auto d = pData->d + (sinl(t-64, 64)+64) * (pData->l) / 32;
				auto l1 = (pData->l+60) * cosm(t) / 256 - 60;
				auto l2 = (pData->l-10) * cosm(t) / 256 + 10;

				if(radius_max){
					l1 = min(radius_max, abs(l1));
					l2 = min(radius_max, abs(l2));
				}

				x = ox + cosl(d, l1);
				y = oy + sinl(d, l2);
			}
			else{
				x = ox + pData->x;
				y = oy + pData->y;
			}
			if(x < 0 || x > 639 || y < 0 || y > 399) continue;

			pPixel[x + (y * ddsd.dwWidth)] = pData->c;
		}
	}, DxObj.PixelFormat);

	DxObj.Back->Unlock(NULL);
}
