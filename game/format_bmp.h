/*
 *   .BMP file format
 *
 */

#pragma once

#include "platform/buffer.h"
#include "platform/unicode.h"
#include "game/coords.h"

// Platform-independent .BMP header types
// --------------------------------------
// Yup, these actually need to be packed.
#pragma pack(push, 1)

// Same as the standard Win32 BITMAPFILEHEADER structure, renamed to avoid
// collisions.
struct BMP_FILEHEADER {
	uint16_t bfType;
	uint32_t bfSize;
	uint16_t bfReserved1;
	uint16_t bfReserved2;
	uint32_t bfOffBits;
};

// Same as the standard Win32 BITMAPINFOHEADER structure, renamed to avoid
// collisions.
struct BMP_INFOHEADER {
	uint32_t biSize;
	int32_t biWidth;
	int32_t biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	int32_t biXPelsPerMeter;
	int32_t biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;

	uint32_t Stride() const {
		return ((((biWidth * biBitCount) + 31u) & ~31) / 8u);
	}
};

// Same as the standard Win32 RGBQUAD structure, renamed to avoid collisions.
struct BGRA {
	uint8_t b;
	uint8_t g;
	uint8_t r;
	uint8_t a;
};

#pragma pack(pop)
// --------------------------------------

// A valid .BMP buffer, with convenient references to the header, optional
// palette, and pixel data inside the buffer.
struct BMP_OWNED {
	BYTE_BUFFER_OWNED buffer;
	const BMP_INFOHEADER& info;
	std::span<BGRA> palette; // Empty if not palettized.
	std::span<std::byte> pixels; // Exactly as large as the image.
};

// Can be safely used for static allocations.
constexpr uint16_t BMP_PALETTE_SIZE_MAX = 256;

// Returns a value between 0 and [BMP_PALETTE_SIZE_MAX].
uint16_t BMPPaletteSizeFromBPP(uint8_t bpp);

std::optional<BMP_OWNED> BMPLoad(BYTE_BUFFER_OWNED buffer);

bool BMPSave(
	const PATH_LITERAL s,
	PIXEL_SIZE size,
	uint16_t planes,
	uint16_t bpp,
	std::span<BGRA> palette,
	std::span<const std::byte> pixels
);
