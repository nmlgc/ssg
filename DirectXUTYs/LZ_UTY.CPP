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

void BIT_DEVICE::Compress(BIT_DEVICE *out,fil_no_t filno) const
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

uint8_t *BIT_DEVICE::MemExpand(fil_no_t filno) const
{
	return nullptr;
}

void BIT_DEVICE::WriteHead() const
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

void BIT_DEVICE::BitDevRelease()
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

void BIT_DEVICE::FilEnd()
{
}
