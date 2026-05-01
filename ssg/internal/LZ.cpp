/*
 *   Packfiles and decompression
 *
 *
 */

#include "ssg/internal/LZ.hpp"

fil_checksum_t FilChecksumAddFile(
	fil_checksum_t& current_total,
	fil_size_t offset,
	fil_size_t size_uncompressed,
	const std::span<const uint8_t> compressed
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

uint8_t BIT_DEVICE_READ::GetBit()
{
	if(cursor.byte >= buffer.size()) {
		return 0xFF;
	}
	const bool ret = ((buffer[cursor.byte] >> (7 - cursor.bit)) & 1);
	cursor += 1;
	return ret;
}

uint32_t BIT_DEVICE_READ::GetBits(size_t bitcount)
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

std::optional<BUFFER_BORROWED> PACKFILE_READ::GetCompressed(
	fil_no_t filno
) const
{
	if(filno >= info.size()) {
		return std::nullopt;
	}
	const size_t start = info[filno].offset;
	const auto end = ((filno == (info.size() - 1))
		? packfile.size()
		: size_t{ info[filno + 1].offset }
	);
	if((start >= packfile.size()) || (end > packfile.size())) {
		return std::nullopt;
	}
	return BUFFER_BORROWED{ packfile.subspan(start, (end - start)) };
}

BUFFER_OWNED PACKFILE_READ::MemExpand(fil_no_t filno) const
{
	const auto maybe_compressed = GetCompressed(filno);
	if(!maybe_compressed) {
		return nullptr;
	}

	const uint32_t size_uncompressed = info[filno].size_uncompressed;
	BUFFER_OWNED uncompressed = { size_uncompressed, BUFFER_HEAP_LOGIC };
	if(!uncompressed) {
		return nullptr;
	}

	// Textbook LZSS.
	std::array<uint8_t, (1 << LZSS_DICT_BITS)> dict;
	fil_size_t out_i = 0;

	auto output = [&uncompressed, &dict, &out_i](uint8_t literal) {
		uncompressed.get()[out_i] = literal;
		dict[out_i & LZSS_DICT_MASK] = literal;
		out_i++;
	};

	BIT_DEVICE_READ device = { maybe_compressed.value() };
	while(out_i < info[filno].size_uncompressed) {
		const bool is_literal = device.GetBit();
		if(is_literal) {
			output(device.GetBits(8));
		} else {
			auto seq_offset = device.GetBits(LZSS_DICT_BITS);
			if(seq_offset == 0) {
				break;
			} else {
				seq_offset--;
			}
			const auto seq_length = (
				device.GetBits(LZSS_SEQ_BITS) + LZSS_SEQ_MIN
			);
			for(auto i = decltype(seq_length){0}; i < seq_length; i++) {
				output(dict[seq_offset++ & LZSS_DICT_MASK]);
			}
		}
	}

	return uncompressed;
}

PACKFILE_READ FilStartR(BUFFER_BORROWED packfile)
{
	BUFFER_CURSOR packfile_cursor = { packfile };

	// PBG_FILEHEAD
	const auto maybe_head = packfile_cursor.next<PBG_FILEHEAD>();
	if(!maybe_head) {
		return {};
	}
	const auto& head = maybe_head.value()[0];
	if(head.name != PBG_HEADNAME) {
		return {};
	}

	// PBG_FILEINFO
	const auto maybe_info = packfile_cursor.next<PBG_FILEINFO>(head.n);
	if(!maybe_info) {
		return {};
	}
	const auto info = maybe_info.value();
	const PACKFILE_READ ret = { packfile, info };

	// Checksums
	fil_checksum_t total_checksum = 0;
	for(fil_no_t i = 0; i < info.size(); i++) {
		const auto maybe_compressed = ret.GetCompressed(i);
		if(!maybe_compressed) {
			return {};
		}
		const auto checksum = FilChecksumAddFile(
			total_checksum,
			info[i].offset,
			info[i].size_uncompressed,
			maybe_compressed.value()
		);
		if(checksum != info[i].checksum_compressed) {
			return {};
		}
	}
	if(total_checksum != head.sum) {
		return {};
	}

	return ret;
}
