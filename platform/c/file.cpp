/*
 *   File loading
 *
 */

#include "platform/file.h"
#include <io.h>

static size_t LoadInplace(std::span<uint8_t> buf, FILE*&& fp)
{
	if(!fp) {
		return 0;
	}
	auto bytes_read = fread(buf.data(), 1, buf.size_bytes(), fp);
	fclose(fp);
	return bytes_read;
}

size_t FileLoadInplace(std::span<uint8_t> buf, const char *s)
{
	return LoadInplace(buf, fopen(s, "rb"));
}

BYTE_BUFFER_OWNED FileLoad(const char *s, size_t size_limit)
{
	auto fp = fopen(s, "rb");
	if(!fp) {
		return {};
	}

	auto size64 = _filelengthi64(fileno(fp));
	if(size64 > size_limit) {
		return {};
	}
	const size_t size = size64;

	auto buf = BYTE_BUFFER_OWNED{ size };
	if(!buf) {
		return {};
	}

	if(LoadInplace({ buf.get(), size }, std::move(fp)) != size) {
		return {};
	}

	return std::move(buf);
}
