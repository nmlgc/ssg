/*
 *   Packfiles and compression
 *
 */

#include "platform/file.h"
#include "LZ_UTY.H"
#include <numeric>

constexpr auto LZSS_DICT_BITS = 13;
constexpr auto LZSS_SEQ_BITS = 4;
constexpr auto LZSS_SEQ_MIN = 3;
constexpr auto LZSS_DICT_MASK = ((1 << LZSS_DICT_BITS) - 1);
constexpr auto LZSS_SEQ_MAX = (LZSS_SEQ_MIN + ((1 << LZSS_SEQ_BITS) - 1));

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
	const BYTE_BUFFER_OWNED& packfile,
	const std::span<const PBG_FILEINFO> info,
	fil_no_t filno
)
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
	return BYTE_BUFFER_BORROWED{ (packfile.get() + start), (end - start) };
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

void BIT_DEVICE_WRITE::PutBit(uint8_t bit)
{
	auto& byte = ((bit_cursor == 0)
		? buffer.emplace_back(0x00)
		: buffer.back()
	);
	byte |= ((bit & 1) << (7 - bit_cursor));
	bit_cursor++;
}

void BIT_DEVICE_WRITE::PutBits(uint32_t bits,unsigned int bitcount)
{
	uint32_t mask = (1 << (bitcount - 1));
	for(decltype(bitcount) i = 0; i < bitcount; i++) {
		PutBit((bits & mask) != 0);
		mask >>= 1;
	}
}

bool BIT_DEVICE_WRITE::Write(const PATH_LITERAL s) const
{
	return FileWrite(s, buffer);
}

BYTE_BUFFER_OWNED PACKFILE_READ::MemExpand(fil_no_t filno) const
{
	const auto maybe_compressed = FilFileGetCompressed(packfile, info, filno);
	if(!maybe_compressed) {
		return nullptr;
	}

	const uint32_t size_uncompressed = info[filno].size_uncompressed;
	BYTE_BUFFER_OWNED uncompressed = { size_uncompressed };
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

BYTE_BUFFER_GROWABLE Compress(BYTE_BUFFER_BORROWED buffer)
{
	constexpr auto DICT_WINDOW = ((1 << LZSS_DICT_BITS) - LZSS_SEQ_MAX);

	BIT_DEVICE_WRITE device;
	fil_size_t in_i = 0;

	while(in_i < buffer.size()) {
		unsigned int seq_offset = 0;
		unsigned int seq_length = 0;
		unsigned int dict_i = ((in_i > DICT_WINDOW) ? (in_i - DICT_WINDOW) : 1);
		while((dict_i < in_i) && (seq_length < LZSS_SEQ_MAX)) {
			unsigned int length_new = 0;
			while(length_new < LZSS_SEQ_MAX) {
				if((in_i + length_new) >= buffer.size()) {
					break;
				}
				if(buffer[dict_i + length_new] != buffer[in_i + length_new]) {
					break;
				}
				length_new++;
			}
			if((length_new > seq_length)) {
				// ([seq_offset] == LZSS_DICT_MASK) would cause the in-file
				// offset to overflow back to 0, which is interpreted as the
				// "sentinel offset" that causes the original game to stop
				// decompressing.
				// (That's why offsets are 1-based to begin with.)
				if((dict_i & LZSS_DICT_MASK) != LZSS_DICT_MASK) {
					seq_length = length_new;
					seq_offset = dict_i;
				}
			}
			dict_i++;
		}
		if(seq_length < LZSS_SEQ_MIN) {
			auto literal = buffer[in_i];
			device.PutBit(true);
			device.PutBits(literal, 8);
			in_i++;
		} else {
			device.PutBit(false);
			device.PutBits((seq_offset + 1), LZSS_DICT_BITS);
			device.PutBits((seq_length - LZSS_SEQ_MIN), LZSS_SEQ_BITS);
			in_i += seq_length;
		}
	}

	// Write the sentinel offset
	device.PutBit(false);
	device.PutBits(0, LZSS_DICT_BITS);

	return device.buffer;
}

bool PACKFILE_WRITE::Write(const PATH_LITERAL s) const
{
	PBG_FILEHEAD head = { .n = files.size() };
	std::vector<PBG_FILEINFO> info(files.size());
	fil_checksum_t sum = 0; // in native byte order

	auto write_header = [&head, &info](FILE_STREAM_WRITE &stream) {
		return (
			stream.Write(std::span(&head, 1)) &&
			stream.Write(std::span(info))
		);
	};

	auto stream = FileStreamWrite(s);
	if(!stream) {
		return false;
	}

	// Write temporary header
	if(!write_header(*stream)) {
		return false;
	}

	// Compress and write files
	for(fil_no_t i = 0; i < files.size(); i++) {
		auto compressed = Compress(files[i]);

		const auto maybe_offset = stream->Tell();
		if(!maybe_offset) {
			return false;
		}

		info[i].offset = maybe_offset.value();
		info[i].size_uncompressed = files[i].size();
		info[i].checksum_compressed = FilChecksumAddFile(
			sum, info[i].offset, info[i].size_uncompressed, compressed
		);
		if(!stream->Write(compressed)) {
			return false;
		}
	}

	// Write real header
	head.sum = sum;
	return (stream->Seek(0, SEEK_WHENCE::BEGIN) && write_header(*stream));
}

BIT_FILE_READ BitFilCreateR(const PATH_LITERAL s)
{
	return { FileLoad(s) };
}

PACKFILE_READ FilStartR(BYTE_BUFFER_OWNED packfile)
{
	auto packfile_cursor = packfile.cursor();

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

	// Checksums
	fil_checksum_t total_checksum = 0;
	for(fil_no_t i = 0; i < info.size(); i++) {
		const auto maybe_compressed = FilFileGetCompressed(packfile, info, i);
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

	return { std::move(packfile), info };
}

PACKFILE_READ FilStartR(std::unique_ptr<FILE_STREAM_READ> stream)
{
	auto ret = FilStartR(stream->ReadAll());
	stream.reset();
	return ret;
}

PACKFILE_READ FilStartR(const PATH_LITERAL s)
{
	return FilStartR(FileLoad(s));
}
