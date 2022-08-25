/*
 *   Packfiles and compression
 *
 */

#include "LZ_UTY.H"
#include <filesystem>

PBG_FILEHEAD FileHead;
PBG_FILEINFO *FileInfo;

std::optional<FILE_BUFFER> FilBufferCreateR(const char *s)
{
	auto fp = fopen(s, "rb");
	if(!fp) {
		return std::nullopt;
	}

	auto size64 = _filelengthi64(fileno(fp));
	if(size64 > std::numeric_limits<fil_size_t>::max()) {
		return std::nullopt;
	}
	const fil_size_t size = size64;

	auto buf = BYTE_BUFFER_OWNED{ new (std::nothrow) uint8_t[size] };
	if(!buf) {
		return std::nullopt;
	}

	if(fread(buf.get(), size, 1, fp) != 1) {
		return std::nullopt;
	}
	fclose(fp);

	return FILE_BUFFER{ std::move(buf), size };
}

uint8_t BIT_DEVICE_READ::GetBit()
{
	if(cursor.byte >= buffer.size()) {
		return 0xFF;
	}
	bool ret = ((buffer[cursor.byte] >> (7 - cursor.bit)) & 1);
	cursor += 1;
	return ret;
}

uint32_t BIT_DEVICE_READ::GetBits(unsigned int bitcount)
{
	const auto bytes_remaining = (buffer.size() - cursor.byte);
	if((bitcount > 24) || (bytes_remaining == 0)) {
		return 0xFFFFFFFF;
	}

	if(((bitcount + 7) / 8) >= bytes_remaining) {
		bitcount = std::min(((bytes_remaining * 8) - cursor.bit), bitcount);
	}
	const auto window_size = (cursor.bit + bitcount);

	uint32_t window = (buffer[cursor.byte + 0] << 24);
	if((bitcount > 1) && (window_size > 8)) {
		window |= (buffer[cursor.byte + 1] << 16);
	}
	if((bitcount > 9) && (window_size > 16)) {
		window |= (buffer[cursor.byte + 2] <<  8);
	}
	if((bitcount > 17) && (window_size > 24)) {
		window |= (buffer[cursor.byte + 3] <<  0);
	}
	window <<= cursor.bit;
	cursor += bitcount;
	return (window >> (32 - bitcount));
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

std::optional<BIT_FILE_READ> BitFilCreateR(const char *s)
{
	auto maybe_file = FilBufferCreateR(s);
	if(!maybe_file) {
		return std::nullopt;
	}
	return std::make_optional<BIT_FILE_READ>(std::move(maybe_file.value()));
}

BIT_DEVICE_WRITE *BitFilCreateW(const char *s)
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
