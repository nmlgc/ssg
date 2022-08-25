/*
 *   Packfiles and compression
 *
 */

#include "LZ_UTY.H"

PBG_FILEHEAD FileHead;
PBG_FILEINFO *FileInfo;

uint8_t BIT_DEVICE_READ::GetBit()
{
	return false;
}

uint32_t BIT_DEVICE_READ::GetBits(unsigned int bitcount)
{
	return 0;
}

void BIT_DEVICE_READ::Compress(PACKFILE_WRITE &out,fil_no_t filno) const
{
}

void BIT_DEVICE_WRITE::PutBit(uint8_t bit)
{
}

void BIT_DEVICE_WRITE::PutBits(uint32_t bits,unsigned int bitcount)
{
}

void BIT_DEVICE_WRITE::PutChar(char c)
{
}

uint8_t *PACKFILE_READ::MemExpand(fil_no_t fileno) const
{
	return nullptr;
}

void PACKFILE_WRITE::WriteHead() const
{
}

BIT_DEVICE_READ *BitFilCreateR(const char *s)
{
	return nullptr;
}

BIT_DEVICE_WRITE *BitFilCreateW(const char *s)
{
	return nullptr;
}

BIT_DEVICE_READ *BitMemCreateR(uint8_t *mem,size_t size)
{
	return nullptr;
}

PACKFILE_READ *FilStartR(const char *s)
{
	return nullptr;
}

PACKFILE_WRITE *FilStartW(const char *s,fil_no_t filecount)
{
	return nullptr;
}
