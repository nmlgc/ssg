/*
 *   Unused double-spiral moiré animation
 *
 */

#include "FONTUTY.H"
#include "DirectXUTYs/UT_MATH.H"
#include "game/coords.h"

// ヒミツの関数 //
void __FillExpoint(EXPOINT *p, int x, int y, int w, int h);

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
