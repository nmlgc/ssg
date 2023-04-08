/*                                                                           */
/*   Lens.cpp   レンズエフェクト                                             */
/*                                                                           */
/*                                                                           */

#include "LENS.H"



// 秘密の関数 //
static BYTE *__LSurfLock(LensInfo *LInfo);
static void __LSurfUnlock(LensInfo *LInfo);

static bool __BackSurfLock(void);
static void __BackSurfUnlock(void);

static BYTE		*__GrpBack = NULL;
static DWORD	__GrpPitch = 0;



// LInfo->gTemp に対して->Lock() を行う  (return : 書き込み先) //
static BYTE *__LSurfLock(LensInfo *LInfo)
{
	DDSURFACEDESC	ddsd;
	HRESULT			ddret;

	memset(&ddsd,0,sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(ddsd);
	ddret = LInfo->gTemp->Lock(NULL,&ddsd,DDLOCK_WAIT,NULL);
	if(ddret != DD_OK) return NULL;

	// The pitch can be different from (width * BPP) here!
	LInfo->gPitch = ddsd.lPitch;

	return (BYTE *)ddsd.lpSurface;
}


// LInfo->gTemp に対して、->Unlock() を行う //
static void __LSurfUnlock(LensInfo *LInfo)
{
	LInfo->gTemp->Unlock(NULL);
}


static bool __BackSurfLock(void)
{
	DDSURFACEDESC	ddsd;
	HRESULT			ddret;

	memset(&ddsd,0,sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(ddsd);
	ddret = DxObj.Back->Lock(NULL,&ddsd,DDLOCK_WAIT,NULL);
	if(ddret != DD_OK) return FALSE;

	__GrpPitch = ddsd.lPitch;
	__GrpBack  = (BYTE *)ddsd.lpSurface;

	return TRUE;
}


static void __BackSurfUnlock(void)
{
	DxObj.Back->Unlock(NULL);
}


// 半径:r  出っ張り:m  のレンズを作成 //
LensInfo* GrpCreateLensBall(uint16_t r, uint16_t m, GRP Surface)
{
	LensInfo	*NewLens;
	DWORD		*Table;
	DWORD		DataSize;
	DWORD		Pitch;
	int	dx,z,w;

	// Since the surface pitch can be different than its with, [Table] will
	// still contain byte offsets, regardless of our main pixel format.
	const auto BitWeight = std::visit([](auto pixel) {
		return sizeof(pixel);
	}, DxObj.PixelFormat);

	if(r <= 0) return NULL;		// 半径がおかしい
	if(r <= m) return NULL;		// 出っ張りは半径より小さくせよ！

	NewLens = (LensInfo *)LocalAlloc(LPTR, sizeof(LensInfo));
	if(NewLens == NULL){
		return NULL;
	}

	const auto Diameter = (int32_t{ r } * 2);
	DataSize = sizeof(DWORD) * Diameter * Diameter;
	NewLens->Data = (DWORD *)LocalAlloc(LPTR, DataSize);
	if(NewLens->Data == NULL){
		LocalFree(NewLens);
		return NULL;
	}

	NewLens->gTemp = Surface;

	NewLens->r      = r;
	NewLens->Height = Diameter;

	__LSurfLock(NewLens);
	__LSurfUnlock(NewLens);
	Pitch = NewLens->gPitch;

	Table = NewLens->Data;
	const auto r2 = (int32_t{ r } * r);
	const auto s = isqrt(r2 - (int32_t{ m } * m));

	for(auto i = -int32_t{ r }; i < r; i++){
		// ｘ座標の測定だ //
		dx = (s * s) - (i * i);

		if(dx > 0){		// 円の内部
			dx = isqrt(dx);
			*Table = w = dx*2;				Table++;	// Width
			*Table = (r-dx)*BitWeight;		Table++;	// Dx
		}
		else{			// 円の外部
			w      = 0;
			*Table = 0;		Table++;		// Width
			*Table = 0;		Table++;		// Dx
		}

		while(w--){
			z = (dx - w) * (dx - w);
			z = isqrt(r2 - z - (i*i));

			*Table = (i * m)/z + r;								// ｙ座標
			*Table = (*Table) * Pitch;							// 幅を掛ける
			*Table = (*Table) + (((dx-w) * m)/z + r)*BitWeight;	// ｘ座標

			Table++;
		}
	}

	return NewLens;
}


// GrpLock() 系関数 : レンズボールを描画する //
extern void GrpDrawLensBall(LensInfo *LInfo, int x, int y)
{
	RECT src = {
		(x - LInfo->r), (y - LInfo->r), (x + LInfo->r), (y + LInfo->r)
	};
	for(;;){
		auto ddrval = LInfo->gTemp->BltFast(0, 0,DxObj.Back,&src,DDBLTFAST_NOCOLORKEY);
		if(ddrval == DD_OK){
			break;
		}
		if(ddrval == DDERR_SURFACELOST){
			if(!GrpRestore())
			return;
		}
		if(ddrval != DDERR_WASSTILLDRAWING){
			return;
		}
	}

	// (x,y) が中心になるように補正する //
	x -= LInfo->r;
	y -= LInfo->r;

	if(x<0 || y<0 || x+LInfo->Height>639 || y+LInfo->Height>479) return;

	// 転送先はロックされているかな //
	if(!__BackSurfLock()) return;
//	GrpLock();
//	if(Grp_Vram == NULL) return;

	auto* Src = __LSurfLock(LInfo);
	if(Src == NULL){
		__BackSurfUnlock();
		return;
	}

	std::visit([&LInfo, &x, &y, &Src](auto pixel) {
		auto* Table = LInfo->Data; // テーブル参照用
		auto* Dest = &__GrpBack[(y * __GrpPitch) + (x * sizeof(pixel))];

		for(decltype(LInfo->Height) row = 0; row < LInfo->Height; row++) {
			auto Width = *(Table++);
			auto* p = reinterpret_cast<decltype(pixel) *>(Dest + *(Table++));

			while(Width--){
				*(p++) = *reinterpret_cast<decltype(pixel) *>(&Src[*(Table++)]);
			}
			Dest += __GrpPitch;
		}
	}, DxObj.PixelFormat);

	__LSurfUnlock(LInfo);
	__BackSurfUnlock();
	//GrpUnlock();
}


// レンズボール用データを破棄する //
extern void GrpReleaseLensBall(LensInfo *LInfo)
{
	if(LInfo == NULL) return;

	if(LInfo->Data) LocalFree(LInfo->Data);

	LocalFree(LInfo);
}
