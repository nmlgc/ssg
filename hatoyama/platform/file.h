/*
 *   File I/O
 *
 */

#pragma once

#include "platform/buffer.h"

struct SDL_IOStream;
struct FILE_TIMESTAMPS {
};

// Retrieves the system-specific timestamps of the given file if it exists, or
// a `nullptr` otherwise.
std::unique_ptr<FILE_TIMESTAMPS> File_TimestampsGet(const char8_t *fn);

// Closes [context] and applies the given timestamps to the on-disk file if
// they are a valid pointer.
bool File_CloseWithTimestamps(
	SDL_IOStream *&& context, std::unique_ptr<FILE_TIMESTAMPS> maybe_timestamps
);

// SDL wrappers
// ------------

SDL_IOStream *SDL_IOFromFile(const char8_t *file, const char *mode);
BYTE_BUFFER_OWNED SDL_LoadFile(const char8_t *file);
BYTE_BUFFER_OWNED SDL_LoadFile_IO(SDL_IOStream *src, bool closeio);
bool SDL_MustReadIO(SDL_IOStream *context, void *ptr, size_t size);
bool SDL_MustWriteIO(SDL_IOStream *context, const void *ptr, size_t size);
bool SDL_SaveFile(const char8_t *file, const void *data, size_t datasize);
// ------------
