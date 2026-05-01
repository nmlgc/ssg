/*
 *   Packfiles and compression
 *
 */

#include <SDL3/SDL_iostream.h>

#include "LZ_UTY.H"
#include "ssg/internal/LZ.hpp"
#include "hatoyama/engine/file.h"
#include "hatoyama/logic/defer.h"

void BIT_DEVICE_WRITE::PutBit(uint8_t bit)
{
	auto& byte = ((bit_cursor == 0)
		? buffer.emplace_back(0x00)
		: buffer.back()
	);
	byte |= ((bit & 1) << (7 - bit_cursor));
	bit_cursor++;
}

void BIT_DEVICE_WRITE::PutBits(uint32_t bits,unsigned int bitcount)
{
	uint32_t mask = (1 << (bitcount - 1));
	for(decltype(bitcount) i = 0; i < bitcount; i++) {
		PutBit((bits & mask) != 0);
		mask >>= 1;
	}
}

bool BIT_DEVICE_WRITE::Write(const char8_t *s) const
{
	return SDL_SaveFile(s, buffer.data(), buffer.size());
}

BUFFER_GROWABLE Compress(BUFFER_BORROWED buffer)
{
	constexpr auto DICT_WINDOW = ((1 << LZSS_DICT_BITS) - LZSS_SEQ_MAX);

	BIT_DEVICE_WRITE device;
	fil_size_t in_i = 0;

	while(in_i < buffer.size()) {
		unsigned int seq_offset = 0;
		unsigned int seq_length = 0;
		unsigned int dict_i = ((in_i > DICT_WINDOW) ? (in_i - DICT_WINDOW) : 1);
		while((dict_i < in_i) && (seq_length < LZSS_SEQ_MAX)) {
			unsigned int length_new = 0;
			while(length_new < LZSS_SEQ_MAX) {
				if((in_i + length_new) >= buffer.size()) {
					break;
				}
				if(buffer[dict_i + length_new] != buffer[in_i + length_new]) {
					break;
				}
				length_new++;
			}
			if((length_new > seq_length)) {
				// ([seq_offset] == LZSS_DICT_MASK) would cause the in-file
				// offset to overflow back to 0, which is interpreted as the
				// "sentinel offset" that causes the original game to stop
				// decompressing.
				// (That's why offsets are 1-based to begin with.)
				if((dict_i & LZSS_DICT_MASK) != LZSS_DICT_MASK) {
					seq_length = length_new;
					seq_offset = dict_i;
				}
			}
			dict_i++;
		}
		if(seq_length < LZSS_SEQ_MIN) {
			auto literal = buffer[in_i];
			device.PutBit(true);
			device.PutBits(literal, 8);
			in_i++;
		} else {
			device.PutBit(false);
			device.PutBits((seq_offset + 1), LZSS_DICT_BITS);
			device.PutBits((seq_length - LZSS_SEQ_MIN), LZSS_SEQ_BITS);
			in_i += seq_length;
		}
	}

	// Write the sentinel offset
	device.PutBit(false);
	device.PutBits(0, LZSS_DICT_BITS);

	return device.buffer;
}

bool FilWrite(
	const char8_t *s,
	std::span<BUFFER_BORROWED> files,
	std::unique_ptr<FILE_TIMESTAMPS> maybe_timestamps
)
{
	if(files.size() > std::numeric_limits<decltype(PBG_FILEHEAD::n)>::max()) {
		return false;
	}
	fil_no_t file_count = files.size();
	PBG_FILEHEAD head = { .n = file_count };
	std::vector<PBG_FILEINFO> info(file_count);
	fil_checksum_t sum = 0; // in native byte order

	auto write_header = [&head, &info](SDL_IOStream *stream) {
		return (
			SDL_MustWriteIO(stream, &head, sizeof(head)) &&
			SDL_MustWriteIO(
				stream, info.data(), (info.size() * sizeof(PBG_FILEINFO))
			)
		);
	};

	auto *stream = SDL_IOFromFile(s, "wb");
	if(!stream) {
		return false;
	}
	defer(
		File_CloseWithTimestamps(std::move(stream), std::move(maybe_timestamps))
	);

	// Write temporary header
	if(!write_header(stream)) {
		return false;
	}

	// Compress and write files
	for(fil_no_t i = 0; i < file_count; i++) {
		auto compressed = Compress(files[i]);

		const auto offset = SDL_TellIO(stream);
		if(offset == -1) {
			return false;
		}

		info[i].offset = offset;
		info[i].size_uncompressed = files[i].size();
		info[i].checksum_compressed = FilChecksumAddFile(
			sum, info[i].offset, info[i].size_uncompressed, compressed
		);
		if(!SDL_MustWriteIO(stream, compressed.data(), compressed.size())) {
			return false;
		}
	}

	// Write real header
	head.sum = sum;
	return (
		(SDL_SeekIO(stream, 0, SDL_IO_SEEK_SET) != -1) && write_header(stream)
	);
}
