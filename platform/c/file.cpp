/*
 *   File loading
 *
 */

#include "platform/file.h"
#include <io.h>

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

	auto elements_read = fread(buf.get(), size, 1, fp);
	fclose(fp);
	if(elements_read != 1) {
		return {};
	}

	return std::move(buf);
}
