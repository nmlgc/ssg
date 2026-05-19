/*
 *   Bit stream reading and packfile decompression
 *
 *   Efficient compression requires either growable buffers or buffered file
 *   I/O, both of which requires a C or C++ runtime of some sort.
 */

#pragma once

#include "hatoyama/api/export.h"
#include "hatoyama/logic/buffer.h"
#include "hatoyama/logic/endian.h"

// Format
// ------

using fil_checksum_t = uint32_t;
using fil_size_t = uint32_t;
using fil_no_t = uint32_t;
constexpr const std::array<char, 4> PBG_HEADNAME = { 'P', 'B', 'G', 0x1A };

struct PBG_FILEHEAD {
	std::array<char, PBG_HEADNAME.size()> name = PBG_HEADNAME;
	ENDIAN_LITTLE<fil_checksum_t> sum = 0;
	ENDIAN_LITTLE<fil_no_t> n = 0;
};

struct PBG_FILEINFO {
	ENDIAN_LITTLE<fil_size_t> size_uncompressed;
	ENDIAN_LITTLE<fil_size_t> offset;
	ENDIAN_LITTLE<fil_checksum_t> checksum_compressed;
};

constexpr auto LZSS_DICT_BITS = 13;
constexpr auto LZSS_SEQ_BITS = 4;
constexpr auto LZSS_SEQ_MIN = 3;
constexpr auto LZSS_DICT_MASK = ((1 << LZSS_DICT_BITS) - 1);
constexpr auto LZSS_SEQ_MAX = (LZSS_SEQ_MIN + ((1 << LZSS_SEQ_BITS) - 1));

HATOYAMA_API fil_checksum_t FilChecksumAddFile(
	fil_checksum_t& current_total,
	fil_size_t offset,
	fil_size_t size_uncompressed,
	const std::span<const uint8_t> compressed
);
// ------

class HATOYAMA_API BIT_DEVICE_READ {
	struct {
		size_t byte = 0;
		uint8_t bit = 0;

		void operator +=(unsigned int bitcount) {
			bit += bitcount;
			byte += (bit / 8);
			bit %= 8;
		}
	} cursor;

public:
	const BUFFER_BORROWED buffer;

	BIT_DEVICE_READ(const BUFFER_BORROWED buffer) :
		buffer(buffer) {
	}

	// Returns 0xFF if we're at the end of the stream.
	uint8_t GetBit();

	// Returns 0xFFFFFFFF if we're at the end of the stream. Supports a maximum
	// of 24 bits.
	uint32_t GetBits(size_t bitcount);
};

struct HATOYAMA_API PACKFILE_READ {
	BUFFER_BORROWED packfile;
	std::span<const PBG_FILEINFO> info;

	std::optional<BUFFER_BORROWED> GetCompressed(fil_no_t filno) const;
	BUFFER_OWNED MemExpand(fil_no_t filno) const;

	explicit operator bool() const {
		return packfile.size();
	}
};

HATOYAMA_API PACKFILE_READ FilStartR(BUFFER_BORROWED packfile);
