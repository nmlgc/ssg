/*
 *   .BMP file format
 *
 */

#include "game/format_bmp.h"
#include "platform/file.h"
#include <assert.h>

constexpr uint16_t BMPPaletteSizeFromBPP(uint8_t bpp)
{
	const auto ret = [bpp]() -> uint16_t {
		if(bpp <= 4) {
			return (1 << 4);
		} else if(bpp <= 8) {
			return (1 << 8);
		}
		return 0;
	}();
	assert(ret <= BMP_PALETTE_SIZE_MAX);
	return ret;
}

std::optional<BMP_OWNED> BMPLoad(BYTE_BUFFER_OWNED buffer)
{
	if(!buffer) {
		return std::nullopt;
	}

	// Needs to be mutable because the game needs to enforce palette color 0 to
	// always be #000000...
	auto cursor = buffer.cursor_mut();

	const auto& maybe_header_file = cursor.next<BMP_FILEHEADER>();
	if(!maybe_header_file) {
		return std::nullopt;
	}
	const auto& header_file = maybe_header_file.value()[0];

	if(header_file.bfType != 0x4D42) { // "BM"
		return std::nullopt;
	}

	// It makes sense to run this function on non-.BMP files when doing
	// automatic file type detection, which should have failed in the two
	// checks above. But if we got here, we expect this to be a valid .BMP,
	// and therefore assert() that it is.
	const auto maybe_header_info = cursor.next<BMP_INFOHEADER>();
	if(!maybe_header_info) {
		assert(!"Not a .BMP file?");
		return std::nullopt;
	}
	const auto& header_info = maybe_header_info.value()[0];

	const auto palette_size = BMPPaletteSizeFromBPP(
		header_info.biPlanes * header_info.biBitCount
	);
	auto maybe_palette = cursor.next<BGRA>(palette_size);
	if(!maybe_palette) {
		assert(!"Needs a palette, but doesn't contain a full one?");
		return std::nullopt;
	}
	auto& palette = maybe_palette.value();

	// [header_info.biSizeImage] can be 0, so we have to manually calculate the
	// actual size allocated by CreateDIBSection() by DWORD-aligning the row
	// stride. We can't just take everything from [image] to the end of the
	// buffer because nothing prevents the file from being larger than what
	// CreateDIBSection() allocated. This actually happens with File 22 in
	// GRAPH.DAT (Reimu's faceset).
	const size_t size = (header_info.Stride() * header_info.biHeight);
	cursor.cursor = header_file.bfOffBits;
	auto maybe_pixels = cursor.next<std::byte>(size);
	if(!maybe_pixels) {
		assert(!"Does not contain all pixels?");
		return std::nullopt;
	}
	auto& pixels = maybe_pixels.value();

	return BMP_OWNED{ std::move(buffer), header_info, palette, pixels };
}

bool BMPSave(
	const PATH_LITERAL s,
	PIXEL_SIZE size,
	uint16_t planes,
	uint16_t bpp,
	std::span<BGRA> palette,
	std::span<const std::byte> pixels
)
{
	const BMP_INFOHEADER header_info = {
		.biSize = sizeof(BMP_INFOHEADER),
		.biWidth = size.w,
		.biHeight = size.h,
		.biPlanes = planes,
		.biBitCount = bpp,
		.biCompression = 0, // BI_RGB
		.biSizeImage = pixels.size_bytes(),
		.biClrUsed = palette.size(),
	};
	const uint32_t pixel_offset = (
		sizeof(BMP_FILEHEADER) + header_info.biSize + palette.size_bytes()
	);
	const BMP_FILEHEADER header_file = {
		.bfType = 0x4D42, // "BM"
		.bfSize = pixel_offset,
		.bfReserved1 = 0,
		.bfReserved2 = 0,
		.bfOffBits = pixel_offset,
	};
	auto stream = FileStreamWrite(s);
	assert(stream);
	return (
		stream &&
		stream->Write(header_file) &&
		stream->Write(header_info) &&
		stream->Write(std::as_bytes(palette)) &&
		stream->Write(pixels)
	);
}
