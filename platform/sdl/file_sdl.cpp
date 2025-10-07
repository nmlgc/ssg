/*
 *   SDL wrappers for file I/O
 *
 */

#include <SDL3/SDL_iostream.h>

#include "platform/file.h"

SDL_IOStream* SDL_IOFromFile(const char8_t *file, const char *mode)
{
	return SDL_IOFromFile(std::bit_cast<const char *>(file), mode);
}

BYTE_BUFFER_OWNED SDL_LoadFile(const char8_t *file)
{
	auto *f = SDL_IOFromFile(std::bit_cast<const char *>(file), "rb");
	if(!f) {
		return {};
	}
	return SDL_LoadFile_IO(f, true);
}

BYTE_BUFFER_OWNED SDL_LoadFile_IO(SDL_IOStream *src, bool closeio)
{
	size_t size;
	auto *buf = SDL_LoadFile_IO(src, &size, true);
	if(!buf) {
		return {};
	}
	return { std::move(buf), size };
}

bool SDL_MustReadIO(SDL_IOStream *context, void *ptr, size_t size)
{
	return (SDL_ReadIO(context, ptr, size) == size);
}

bool SDL_MustWriteIO(SDL_IOStream *context, const void *ptr, size_t size)
{
	return (SDL_WriteIO(context, ptr, size) == size);
}

bool SDL_SaveFile(const char8_t *file, const void *data, size_t datasize)
{
	return SDL_SaveFile(std::bit_cast<const char *>(file), data, datasize);
}
