/*                                                                           */
/*   Lens.cpp   レンズエフェクト                                             */
/*                                                                           */
/*                                                                           */

#include "LENS.H"
#include "constants.h"
#include "platform/graphics_backend.h"
#include "game/cast.h"
#include "game/ut_math.h"
#include <assert.h>



// 半径:r  出っ張り:m  のレンズを作成 //
std::optional<LensInfo> GrpCreateLensBall(uint16_t r, uint16_t m)
{
	int	dx,z,w;

	// Since the surface pitch can be different than its with, [Table] will
	// still contain byte offsets, regardless of our main pixel format.
	const auto BitWeight = GrpBackend_PixelFormat().PixelByteSize();

	assert(r > 0);	// 半径がおかしい
	assert(r > m);	// 出っ張りは半径より小さくせよ！

	const uint16_t Diameter = (r * 2);
	const uint16_t area = (Diameter * Diameter);

	LensInfo NewLens = { .r = r, .Height = Diameter };

	NewLens.FOV = std::unique_ptr<std::byte[]>(
		new (std::nothrow) std::byte[area * BitWeight]
	);
	if(NewLens.FOV == nullptr) {
		return std::nullopt;
	}

	// Not even restricting [Diameter] to int8_t would convince Visual Studio's
	// static analyzer that this can't overflow.
	#pragma warning(suppress: 26831)
	NewLens.Data = std::unique_ptr<uint32_t[]>(
		new (std::nothrow) uint32_t[area]
	);
	if(NewLens.Data == nullptr) {
		return std::nullopt;
	}

	auto* Table = NewLens.Data.get();
	const auto r2 = (Cast::up_sign<int32_t>(r) * r);
	const auto s = isqrt(r2 - (Cast::up_sign<int32_t>(m) * m));

	for(auto i = -Cast::up_sign<int32_t>(r); i < r; i++){
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
			*Table = (*Table * Diameter);	// 幅を掛ける
			*Table = (*Table + ((((dx - w) * m) / z) + r));	// ｘ座標

			Table++;
		}
	}

	return NewLens;
}


// GrpLock() 系関数 : レンズボールを描画する //
void LensInfo::Draw(WINDOW_POINT center)
{
	// (x,y) が中心になるように補正する //
	const WINDOW_COORD left = (center.x - r);
	const WINDOW_COORD top = (center.y - r);

	if(
		(left < 0) ||
		(top < 0) ||
		((left + Height) > (GRP_RES.w - 1)) ||
		((top + Height) > (GRP_RES.h - 1))
	) {
		return;
	}

	GrpBackend_PixelAccessEdit([&]<class P>(std::byte *pixels, size_t pitch) {
		const auto fov_buffer = reinterpret_cast<P *>(FOV.get());
		const auto fov_size = (static_cast<size_t>(Height) * Height);
		const std::span fov = { fov_buffer, fov_size };

		auto* Table = Data.get(); // テーブル参照用
		auto* Dest = &pixels[(top * pitch) + (left * sizeof(P))];

		// Capture the pixels under the lens
		const auto* back_p = Dest;
		for(auto fov_p = fov.begin(); fov_p != fov.end(); fov_p += Height) {
			std::copy_n(reinterpret_cast<const P *>(back_p), Height, fov_p);
			back_p += pitch;
		}

		for(decltype(Height) row = 0; row < Height; row++) {
			auto Width = *(Table++);
			auto* p = reinterpret_cast<P *>(Dest + *(Table++));

			while(Width--){
				*(p++) = fov[*(Table++)];
			}
			Dest += pitch;
		}
	});
}
