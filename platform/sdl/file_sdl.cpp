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

bool SDL_MustReadIO(SDL_IOStream *context, void *ptr, size_t size)
{
	return (SDL_ReadIO(context, ptr, size) == size);
}

bool SDL_SaveFile(const char8_t *file, const void *data, size_t datasize)
{
	return SDL_SaveFile(std::bit_cast<const char *>(file), data, datasize);
}
