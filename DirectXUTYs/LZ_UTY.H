/*
 *   Packfiles and compression
 *
 */

#pragma once

#include <array>
#include <stdint.h>

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

struct PACKFILE_WRITE;

struct BIT_DEVICE_READ {
	uint8_t GetBit();
	uint32_t GetBits(unsigned int bitcount);
	void Compress(PACKFILE_WRITE &out,fil_no_t filno) const;
};

struct BIT_DEVICE_WRITE {
	char rack;
	uint8_t mask;

	void PutBit(uint8_t bit);
	void PutBits(uint32_t bits,unsigned int bitcount);
	void PutChar(char c);
};

struct PACKFILE_READ {
	uint8_t *MemExpand(fil_no_t filno) const;
};

struct PACKFILE_WRITE {
	FILE *file;

	void WriteHead() const;
};

extern PBG_FILEHEAD FileHead;
extern PBG_FILEINFO *FileInfo;

BIT_DEVICE_READ *BitFilCreateR(const char *s);
BIT_DEVICE_WRITE *BitFilCreateW(const char *s);
BIT_DEVICE_READ *BitMemCreateR(uint8_t *mem,size_t size);

PACKFILE_READ *FilStartR(const char *s);
PACKFILE_WRITE *FilStartW(const char *s,fil_no_t filno);
