/*
 *   2D surfaces via DirectDraw
 *
 */

#include "platform/windows/surface_ddraw.h"
#include "game/format_bmp.h"
#include <assert.h>
#include <ddraw.h>

bool DDrawSaveSurface(FILE_STREAM_WRITE* stream, IDirectDrawSurface* surf)
{
	if(!surf || !stream) {
		return false;
	}
	std::array<BGRA, BMP_PALETTE_SIZE_MAX> bgra_memory;
	const auto palette = [surf, &bgra_memory]() -> std::span<BGRA> {
		IDirectDrawPalette* pal = nullptr;
		if((surf->GetPalette(&pal) != DD_OK) || !pal) {
			return std::span<BGRA>{};
		}
		std::array<PALETTEENTRY, BMP_PALETTE_SIZE_MAX> rgba;
		if(pal->GetEntries(0, 0, rgba.size(), rgba.data()) != DD_OK) {
			return std::span<BGRA>{};
		}
		for(int i = 0; i < rgba.size(); i++) {
			bgra_memory[i] = {
				.b = rgba[i].peBlue,
				.g = rgba[i].peGreen,
				.r = rgba[i].peRed,
				.a = rgba[i].peFlags
			};
		}
		pal->Release();
		return bgra_memory;
	}();

	DDSURFACEDESC desc = { .dwSize = sizeof(desc) };
	if(surf->Lock(nullptr, &desc, DDLOCK_WAIT, nullptr) != DD_OK) {
		return false;
	} else if(desc.lPitch < 0) {
		assert(!"Negative pitch?");
		return false;
	}

	const uint16_t bpp = desc.ddpfPixelFormat.dwRGBBitCount;
	const PIXEL_SIZE size = {
		// The rows of DirectDraw surfaces tend to be aligned to larger
		// boundaries than the DWORD alignment of .BMP files. Let's just round
		// up the .BMP width to that stride so that we can directly write the
		// surface memory in a single block, without trimming the rows.
		static_cast<int32_t>(desc.lPitch / (bpp / 8)),
		-static_cast<int32_t>(desc.dwHeight)
	};
	const std::span<const std::byte> pixels = {
		static_cast<const std::byte *>(desc.lpSurface),
		static_cast<size_t>(desc.lPitch * desc.dwHeight),
	};
	const auto ret = BMPSave(stream, size, 1, bpp, palette, pixels);
	surf->Unlock(nullptr);
	return ret;
}
