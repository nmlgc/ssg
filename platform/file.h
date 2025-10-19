/*
 *   File I/O
 *
 */

#pragma once

#include "platform/buffer.h"
#include "platform/unicode.h"

enum class FILE_FLAGS : uint8_t {
	_HAS_BITFLAG_OPERATORS,
	NONE = 0x0,
	FAIL_IF_EXISTS = 0x01,
	PRESERVE_TIMESTAMPS = 0x02,
};

// Streams
// -------

enum class SEEK_WHENCE {
	BEGIN, CURRENT, END
};

struct FILE_STREAM {
	virtual ~FILE_STREAM() {};
};

struct FILE_STREAM_SEEK : FILE_STREAM {
	// Returns `true` if the seek was successful.
	[[nodiscard]] virtual bool Seek(int64_t offset, SEEK_WHENCE whence) = 0;

	virtual std::optional<int64_t> Tell() = 0;
};

struct FILE_STREAM_READ : FILE_STREAM_SEEK {
	// Reads the entire file into a newly allocated buffer, leaving the read
	// pointer at the end of the file.
	[[nodiscard]] virtual BYTE_BUFFER_OWNED ReadAll() = 0;
};

struct FILE_STREAM_WRITE : FILE_STREAM_SEEK {
	// Retuns `true` if the buffer was written successfully.
	[[nodiscard]] virtual bool Write(BYTE_BUFFER_BORROWED buf) = 0;
};

std::unique_ptr<FILE_STREAM_READ> FileStreamRead(const PATH_LITERAL s);
std::unique_ptr<FILE_STREAM_READ> FileStreamRead(const char8_t* s);

std::unique_ptr<FILE_STREAM_WRITE> FileStreamWrite(
	const PATH_LITERAL s, FILE_FLAGS flags = FILE_FLAGS::NONE
);
std::unique_ptr<FILE_STREAM_WRITE> FileStreamWrite(
	const char8_t* s, FILE_FLAGS flags = FILE_FLAGS::NONE
);
// -------

// SDL wrappers
// ------------

struct SDL_IOStream;

SDL_IOStream *SDL_IOFromFile(const char8_t *file, const char *mode);
BYTE_BUFFER_OWNED SDL_LoadFile(const char8_t *file);
BYTE_BUFFER_OWNED SDL_LoadFile_IO(SDL_IOStream *src, bool closeio);
bool SDL_MustReadIO(SDL_IOStream *context, void *ptr, size_t size);
bool SDL_MustWriteIO(SDL_IOStream *context, const void *ptr, size_t size);
bool SDL_SaveFile(const char8_t *file, const void *data, size_t datasize);
// ------------
