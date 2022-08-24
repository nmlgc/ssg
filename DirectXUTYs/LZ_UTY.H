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

enum BIT_DEVICE_FILE_MODE {
	BD_FILE_READ,
	BD_FILE_WRITE,
};

struct BIT_DEVICE {
	FILE *file;
	char rack;
	uint8_t mask;
};

extern PBG_FILEHEAD FileHead;
extern PBG_FILEINFO *FileInfo;

uint8_t GetBit(BIT_DEVICE *bd);
uint32_t GetBits(BIT_DEVICE *bd,unsigned int bitcount);
void Compress(const BIT_DEVICE *in,BIT_DEVICE *out,fil_no_t filno);
void PutBit(BIT_DEVICE *bd,uint8_t bit);
void PutBits(BIT_DEVICE *bd,uint32_t bits,unsigned int bitcount);
void PutChar(char c,BIT_DEVICE *bd);

uint8_t *MemExpand(const BIT_DEVICE *in,fil_no_t filno);
void WriteHead(BIT_DEVICE *in);

BIT_DEVICE *BitFilCreate(const char *s,BIT_DEVICE_FILE_MODE mode);
BIT_DEVICE *BitMemCreateR(uint8_t *mem,size_t size);
void BitDevRelease(BIT_DEVICE *bd);

BIT_DEVICE *FilStartR(const char *s);
BIT_DEVICE *FilStartW(const char *s,fil_no_t filecount);
void FilEnd(BIT_DEVICE *bd);
