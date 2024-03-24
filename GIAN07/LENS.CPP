/*                                                                           */
/*   Lens.cpp   レンズエフェクト                                             */
/*                                                                           */
/*                                                                           */

#include "LENS.H"
#include "DirectXUTYs/DD_UTY.H"
#include "game/ut_math.h"
#include <assert.h>
#include <ddraw.h>



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

	memset(&ddsd,0,sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(ddsd);
	const auto ddret = LInfo->gTemp->surf->Lock(
		nullptr, &ddsd, DDLOCK_WAIT, nullptr
	);
	if(ddret != DD_OK) return NULL;

	// The pitch can be different from (width * BPP) here!
	LInfo->gPitch = ddsd.lPitch;

	return static_cast<uint8_t *>(ddsd.lpSurface);
}


// LInfo->gTemp に対して、->Unlock() を行う //
static void __LSurfUnlock(LensInfo *LInfo)
{
	LInfo->gTemp->surf->Unlock(nullptr);
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
	__GrpBack  = static_cast<uint8_t *>(ddsd.lpSurface);

	return TRUE;
}


static void __BackSurfUnlock(void)
{
	DxObj.Back->Unlock(NULL);
}


// 半径:r  出っ張り:m  のレンズを作成 //
std::optional<LensInfo> GrpCreateLensBall(
	uint16_t r, uint16_t m, SURFACE_DDRAW& surf
)
{
	if(!DxObj.SurfaceCreateUninitialized(surf, { (r * 2), (r * 2) })) {
		return std::nullopt;
	}

	LensInfo	NewLens = { .gTemp = &surf };
	int	dx,z,w;

	// Since the surface pitch can be different than its with, [Table] will
	// still contain byte offsets, regardless of our main pixel format.
	const auto BitWeight = std::visit([](auto pixel) {
		return sizeof(pixel);
	}, DxObj.PixelFormat);

	assert(r > 0);	// 半径がおかしい
	assert(r > m);	// 出っ張りは半径より小さくせよ！

	const auto Diameter = (int32_t{ r } * 2);

	// Not even restricting [Diameter] to int8_t would convince Visual Studio's
	// static analyzer that this can't overflow.
	#pragma warning(suppress: 26831)
	NewLens.Data = std::unique_ptr<uint32_t[]>(
		new (std::nothrow) uint32_t[Diameter * Diameter]
	);
	if(NewLens.Data == nullptr) {
		return std::nullopt;
	}

	NewLens.r      = r;
	NewLens.Height = Diameter;

	__LSurfLock(&NewLens);
	__LSurfUnlock(&NewLens);
	const auto Pitch = NewLens.gPitch;

	auto* Table = NewLens.Data.get();
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
void LensInfo::Draw(WINDOW_POINT center)
{
	RECT src = {
		(center.x - r), (center.y - r), (center.x + r), (center.y + r)
	};
	for(;;){
		const auto ddrval = gTemp->surf->BltFast(
			0, 0, DxObj.Back, &src, DDBLTFAST_NOCOLORKEY
		);
		if(ddrval == DD_OK){
			break;
		}
		if(ddrval == DDERR_SURFACELOST){
			if(!DxObj.Restore())
			return;
		}
		if(ddrval != DDERR_WASSTILLDRAWING){
			return;
		}
	}

	// (x,y) が中心になるように補正する //
	const WINDOW_COORD left = (center.x - r);
	const WINDOW_COORD top = (center.y - r);

	if(
		(left < 0) ||
		(top < 0) ||
		((left + Height) > 639) ||
		((top + Height) > 479)
	) {
		return;
	}

	// 転送先はロックされているかな //
	if(!__BackSurfLock()) return;
//	GrpLock();
//	if(Grp_Vram == NULL) return;

	auto* Src = __LSurfLock(this);
	if(Src == NULL){
		__BackSurfUnlock();
		return;
	}

	std::visit([this, &left, &top, &Src](auto pixel) {
		auto* Table = Data.get(); // テーブル参照用
		auto* Dest = &__GrpBack[(top * __GrpPitch) + (left * sizeof(pixel))];

		for(decltype(Height) row = 0; row < Height; row++) {
			auto Width = *(Table++);
			[[gsl::suppress(type.1)]]
			auto* p = reinterpret_cast<decltype(pixel) *>(Dest + *(Table++));

			while(Width--){
				[[gsl::suppress(type.1)]]
				*(p++) = *reinterpret_cast<decltype(pixel) *>(&Src[*(Table++)]);
			}
			Dest += __GrpPitch;
		}
	}, DxObj.PixelFormat);

	__LSurfUnlock(this);
	__BackSurfUnlock();
	//GrpUnlock();
}
