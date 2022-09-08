/*
 *   File loading
 *
 */

#include "platform/file.h"
#include <io.h>

std::optional<BYTE_BUFFER_OWNED> FileLoad(const char *s, size_t size_limit)
{
	auto fp = fopen(s, "rb");
	if(!fp) {
		return std::nullopt;
	}

	auto size64 = _filelengthi64(fileno(fp));
	if(size64 > size_limit) {
		return std::nullopt;
	}
	const size_t size = size64;

	auto buf = BYTE_BUFFER_OWNED{ size };
	if(!buf) {
		return std::nullopt;
	}

	auto elements_read = fread(buf.get(), size, 1, fp);
	fclose(fp);
	if(elements_read != 1) {
		return std::nullopt;
	}

	return std::move(buf);
}
