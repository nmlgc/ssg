/*
 *   Packfiles and compression
 *
 */

#include "LZ_UTY.H"

PBG_FILEHEAD FileHead;
PBG_FILEINFO *FileInfo;

uint8_t BIT_DEVICE::GetBit()
{
	return false;
}

uint32_t BIT_DEVICE::GetBits(unsigned int bitcount)
{
	return 0;
}

void BIT_DEVICE::Compress(PACKFILE_WRITE &out,fil_no_t filno) const
{
}

void BIT_DEVICE::PutBit(uint8_t bit)
{
}

void BIT_DEVICE::PutBits(uint32_t bits,unsigned int bitcount)
{
}

void BIT_DEVICE::PutChar(char c)
{
}

uint8_t *PACKFILE_READ::MemExpand(fil_no_t fileno) const
{
	return nullptr;
}

void PACKFILE_WRITE::WriteHead() const
{
}

BIT_DEVICE *BitFilCreate(const char *s,BIT_DEVICE_FILE_MODE mode)
{
	return nullptr;
}

BIT_DEVICE *BitMemCreateR(uint8_t *mem,size_t size)
{
	return nullptr;
}

PACKFILE_READ *FilStartR(const char *s)
{
	return nullptr;
}

<<<<<<< HEAD
BIT_DEVICE *FilStartW(const char *s,fil_no_t filecount)
=======
PACKFILE_WRITE *FilStartW(const char *s,fil_no_t filecount)
>>>>>>> 03b1516 ([LZ] Distinguish between bit device and read/write packfile types)
{
	return nullptr;
}
