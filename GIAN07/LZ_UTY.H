/*
 *   Packfiles and compression
 *
 */

#pragma once

#include "platform/buffer.h"
#include "platform/unicode.h"
#include <array>

// Format
// ------

using fil_checksum_t = uint32_t;
using fil_size_t = uint32_t;
using fil_no_t = uint32_t;
constexpr const std::array<char, 4> PBG_HEADNAME = { 'P', 'B', 'G', 0x1A };

struct PBG_FILEHEAD {
	std::array<char, PBG_HEADNAME.size()> name = PBG_HEADNAME;
	fil_checksum_t sum = 0;
	fil_no_t n = 0;
};

struct PBG_FILEINFO {
	fil_size_t size_uncompressed;
	fil_size_t offset;
	fil_checksum_t checksum_compressed;
};
// ------

class BIT_DEVICE_READ {
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
	const BYTE_BUFFER_BORROWED buffer;

	BIT_DEVICE_READ(const BYTE_BUFFER_BORROWED buffer) :
		buffer(buffer) {
	}

	BIT_DEVICE_READ(const void* mem,size_t size) :
		buffer({ reinterpret_cast<const uint8_t *>(mem), size }) {
	}

	// Returns 0xFF if we're at the end of the stream.
	uint8_t GetBit();

	// Returns 0xFFFFFFFF if we're at the end of the stream. Supports a maximum
	// of 24 bits.
	uint32_t GetBits(unsigned int bitcount);
};

struct BIT_DEVICE_WRITE {
	BYTE_BUFFER_GROWABLE buffer;
	uint8_t bit_cursor:3 = 0;

	void PutBit(uint8_t bit);
	void PutBits(uint32_t bits,unsigned int bitcount);
	bool Write(const PATH_LITERAL s) const;
};

struct BIT_FILE_READ : public BIT_DEVICE_READ {
	const BYTE_BUFFER_OWNED file;

	BIT_FILE_READ(BYTE_BUFFER_OWNED &&file) :
		BIT_DEVICE_READ(file.get(),file.size()),
		file(std::move(file)) {
	}
};

struct PACKFILE_READ {
	const BYTE_BUFFER_OWNED packfile;
	const std::span<const PBG_FILEINFO> info;

	PACKFILE_READ(std::nullptr_t null = nullptr) {
	}

	PACKFILE_READ(
		BYTE_BUFFER_OWNED &&packfile, const std::span<const PBG_FILEINFO> info
	) :
		packfile(std::move(packfile)),
		info(info) {
	}

	BYTE_BUFFER_OWNED MemExpand(fil_no_t filno) const;
};

struct PACKFILE_WRITE {
	std::vector<BYTE_BUFFER_BORROWED> files;

	bool Write(const PATH_LITERAL s) const;
};

BIT_FILE_READ BitFilCreateR(const PATH_LITERAL s);
PACKFILE_READ FilStartR(const PATH_LITERAL s);
