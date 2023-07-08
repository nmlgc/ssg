/*
 *   2D surfaces via GDI bitmaps
 *
 */

#include "platform/windows/surface_gdi.h"
#include "platform/file.h"
#include "game/format_bmp.h"
#include <array>
#include <assert.h>

SURFACE_GDI::SURFACE_GDI() :
	dc(CreateCompatibleDC(GetDC(nullptr)))
{
}

SURFACE_GDI::~SURFACE_GDI()
{
	Delete();
	DeleteDC(dc);
}

static_assert(sizeof(RGBQUAD) == sizeof(BGRA));
static_assert(offsetof(RGBQUAD, rgbBlue) == offsetof(BGRA, b));
static_assert(offsetof(RGBQUAD, rgbGreen) == offsetof(BGRA, g));
static_assert(offsetof(RGBQUAD, rgbRed) == offsetof(BGRA, r));
static_assert(offsetof(RGBQUAD, rgbReserved) == offsetof(BGRA, a));

bool SURFACE_GDI::Save(const PATH_LITERAL s) const
{
	DIBSECTION dib;
	if(!GetObject(img, sizeof(DIBSECTION), &dib)) {
		return false;
	}

	const auto bpp = (dib.dsBm.bmPlanes * dib.dsBm.bmBitsPixel);
	const auto palette_size = BMPPaletteSizeFromBPP(bpp);

	if(!dib.dsBm.bmBits) {
		// This is a DDB, not a DIB, which means that the BITMAPINFOHEADER
		// structure is invalid. Reconstruct it from what we have.
		// Adapted from
		//
		// 	https://learn.microsoft.com/en-us/windows/win32/gdi/storing-an-image
		const PIXEL_SIZE size = { dib.dsBm.bmWidth, dib.dsBm.bmHeight };

		// GetDIBits() will write the color table after the BITMAPINFOHEADER
		// structure.
		std::vector<std::byte> info_buf(
			sizeof(BITMAPINFOHEADER) + (sizeof(RGBQUAD) * palette_size)
		);

		auto* info = reinterpret_cast<BMP_INFOHEADER *>(info_buf.data());
		std::span<BGRA> palette = {
			reinterpret_cast<BGRA *>(&info[1]), palette_size
		};

		// According to MSDN, we explicitly only need the first 6 members.
		// GetDIBits() overwrites the rest anyway.
		*info = {
			.biSize = sizeof(dib.dsBmih),
			.biWidth = size.w,
			.biHeight = size.h,
			.biPlanes = dib.dsBm.bmPlanes,
			.biBitCount = dib.dsBm.bmBitsPixel,
			.biCompression = BI_RGB,
		};
		info->biSizeImage = (info->Stride() * info->biHeight);

		std::vector<std::byte> pixels(info->biSizeImage);
		auto* bmi = reinterpret_cast<BITMAPINFO *>(info);
		if(!GetDIBits(dc, img, 0, size.h, pixels.data(), bmi, DIB_RGB_COLORS)) {
			assert(!"GetDIBits failed");
			return false;
		}
		return BMPSave(
			s, size, info->biPlanes, info->biBitCount, palette, pixels
		);
	}

	// For DIBs, we get direct access to the pixel data.
	std::array<BGRA, BMP_PALETTE_SIZE_MAX> bgra;
	std::span<BGRA> palette = {};
	const auto color_table_ret = GetDIBColorTable(
		dc, 0, palette_size, reinterpret_cast<RGBQUAD *>(bgra.data())
	);
	if(color_table_ret == palette_size) {
		palette = bgra;
	}
	const std::span<const std::byte> pixels = {
		reinterpret_cast<const std::byte *>(dib.dsBm.bmBits),
		size_t(dib.dsBm.bmWidthBytes * dib.dsBm.bmHeight)
	};
	return BMPSave(
		s,
		{ dib.dsBmih.biWidth, dib.dsBmih.biHeight },
		dib.dsBmih.biPlanes,
		dib.dsBmih.biBitCount,
		palette,
		pixels
	);
}

void SURFACE_GDI::Delete()
{
	if(img) {
		SelectObject(dc, stock_img);
		DeleteObject(img);
		img = nullptr;
	}
}
