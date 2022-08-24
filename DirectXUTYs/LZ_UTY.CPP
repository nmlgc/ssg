/*
 *   Packfiles and compression
 *
 */

#include "LZ_UTY.H"

PBG_FILEHEAD FileHead;
PBG_FILEINFO *FileInfo;

uint8_t GetBit(BIT_DEVICE *bd)
{
	return false;
}

uint32_t GetBits(BIT_DEVICE *bd,unsigned int bitcount)
{
	return 0;
}

void Compress(const BIT_DEVICE *in,BIT_DEVICE *out,fil_no_t filno)
{
}

void PutBit(BIT_DEVICE *bd,uint8_t bit)
{
}

void PutBits(BIT_DEVICE *bd,uint32_t bits,unsigned int bitcount)
{
}

void PutChar(char c,BIT_DEVICE *bd)
{
}

uint8_t *MemExpand(const BIT_DEVICE *in,fil_no_t filno)
{
	return nullptr;
}

void WriteHead(BIT_DEVICE *in)
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

void BitDevRelease(BIT_DEVICE *bd)
{
}

BIT_DEVICE *FilStartR(const char *s)
{
	return nullptr;
}

BIT_DEVICE *FilStartW(const char *s,fil_no_t filecount)
{
	return nullptr;
}

void FilEnd(BIT_DEVICE *bd)
{
}
