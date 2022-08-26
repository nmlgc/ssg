/*
 *   Packfiles and compression
 *
 */

#include "LZ_UTY.H"
#include <io.h>
#include <numeric>

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

template <typename Container> fil_checksum_t FilChecksumAddFile(
	fil_checksum_t &current_total,
	fil_size_t offset,
	fil_size_t size_uncompressed,
	const Container &compressed
)
{
	auto ret = std::accumulate(
		compressed.begin(), compressed.end(), fil_checksum_t{ 0 }
	);
	current_total += ret;
	current_total += size_uncompressed;
	current_total += offset;
	return ret;
}

std::optional<BYTE_BUFFER_BORROWED> FilFileGetCompressed(
	const FILE_BUFFER& packfile,
	const std::span<const PBG_FILEINFO> info,
	fil_no_t filno
)
{
	if(filno >= info.size()) {
		return std::nullopt;
	}
	const auto start = info[filno].offset;
	const auto end = ((filno == (info.size() - 1))
		? packfile.size
		: info[filno + 1].offset
	);
	if((start >= packfile.size) || (end > packfile.size)) {
		return std::nullopt;
	}
	return BYTE_BUFFER_BORROWED{
		(packfile.buffer.get() + start), (end - start)
	};
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

std::optional<PACKFILE_READ> FilStartR(const char *s)
{
	auto maybe_packfile = FilBufferCreateR(s);
	if(!maybe_packfile) {
		return std::nullopt;
	}
	auto& packfile = maybe_packfile.value();

	// PBG_FILEHEAD
	size_t byte_start = 0;
	size_t byte_end = sizeof(PBG_FILEHEAD);
	if(packfile.size < byte_end) {
		return std::nullopt;
	}
	const auto& head = *reinterpret_cast<PBG_FILEHEAD *>(
		(packfile.buffer.get() + byte_start)
	);
	if(head.name != PBG_HEADNAME) {
		return std::nullopt;
	}

	// PBG_FILEINFO
	byte_start = byte_end;
	byte_end = (byte_start + (head.n * sizeof(PBG_FILEINFO)));
	if(packfile.size < byte_end) {
		return std::nullopt;
	}

	const auto* info_start = (packfile.buffer.get() + byte_start);
	decltype(PACKFILE_READ::info) info = {
		reinterpret_cast<const PBG_FILEINFO *>(info_start), head.n
	};
	const BYTE_BUFFER_BORROWED info_bytes = { info_start, info.size_bytes() };

	// Checksums
	fil_checksum_t total_checksum = 0;
	for(fil_no_t i = 0; i < info.size(); i++) {
		const auto maybe_compressed = FilFileGetCompressed(packfile, info, i);
		if(!maybe_compressed) {
			return std::nullopt;
		}
		const auto checksum = FilChecksumAddFile(
			total_checksum,
			info[i].offset,
			info[i].size_uncompressed,
			maybe_compressed.value()
		);
		if(checksum != info[i].checksum_compressed) {
			return std::nullopt;
		}
	}
	if(total_checksum != head.sum) {
		return std::nullopt;
	}

	return std::make_optional<PACKFILE_READ>(std::move(packfile), info);
}

PACKFILE_WRITE *FilStartW(const char *s,fil_no_t filecount)
{
	return nullptr;
}
