/*
 *   .BMP file format
 *
 */

#pragma once

#include "platform/file.h"
#include "game/coords.h"
#include "game/endian.h"

// Platform-independent .BMP header types
// --------------------------------------
// Yup, these actually need to be packed.
#pragma pack(push, 1)

// Same as the standard Win32 BITMAPFILEHEADER structure, renamed to avoid
// collisions.
struct BMP_FILEHEADER {
	U16LE bfType;
	U32LE bfSize;
	U16LE bfReserved1;
	U16LE bfReserved2;
	U32LE bfOffBits;
};

// Same as the standard Win32 BITMAPINFOHEADER structure, renamed to avoid
// collisions.
struct BMP_INFOHEADER {
	U32LE biSize;
	I32LE biWidth;
	I32LE biHeight;
	U16LE biPlanes;
	U16LE biBitCount;
	U32LE biCompression;
	U32LE biSizeImage;
	I32LE biXPelsPerMeter;
	I32LE biYPelsPerMeter;
	U32LE biClrUsed;
	U32LE biClrImportant;

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
constexpr uint16_t BMPPaletteSizeFromBPP(uint8_t bpp);

std::optional<BMP_OWNED> BMPLoad(BYTE_BUFFER_OWNED buffer);

bool BMPSave(
	FILE_STREAM_WRITE* stream,
	PIXEL_SIZE size,
	uint16_t planes,
	uint16_t bpp,
	std::span<BGRA> palette,
	std::span<const std::byte> pixels
);
