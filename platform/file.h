/*
 *   File loading
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

// Fills [buf] with the first [buf.size_bytes()] read from the file with the
// given name, and returns the number of bytes read. Successful if the returned
// value is equal to [buf.size_bytes()].
[[nodiscard]] size_t FileLoadInplace(
	std::span<uint8_t> buf, const PATH_LITERAL s
);
[[nodiscard]] size_t FileLoadInplace(std::span<uint8_t> buf, const char8_t* s);

// Loads the file with the given name into a newly allocated buffer, if
// possible.
BYTE_BUFFER_OWNED FileLoad(
	const PATH_LITERAL s,
	size_t size_limit = (std::numeric_limits<size_t>::max)()
);
BYTE_BUFFER_OWNED FileLoad(
	const char8_t* s, size_t size_limit = (std::numeric_limits<size_t>::max)()
);

// Saves the given sequence of buffers to the file with the given name,
// overwriting the file if it already exists. Returns `true` on success.
bool FileWrite(
	const PATH_LITERAL s, std::span<const BYTE_BUFFER_BORROWED> bufs
);
bool FileWrite(const char8_t* s, std::span<const BYTE_BUFFER_BORROWED> bufs);

// Saves the given buffer to the file with the given name, overwriting the file
// if it already exists. Returns `true` on success.
static bool FileWrite(const PATH_LITERAL s, const BYTE_BUFFER_BORROWED buf) {
	return FileWrite(s, std::span<const BYTE_BUFFER_BORROWED>{ &buf, 1 });
}
static bool FileWrite(const char8_t* s, const BYTE_BUFFER_BORROWED buf) {
	return FileWrite(s, std::span<const BYTE_BUFFER_BORROWED>{ &buf, 1 });
}

// Appends the given sequence of buffers to the end of the given file. Returns
// `true` on success.
bool FileAppend(
	const PATH_LITERAL s, std::span<const BYTE_BUFFER_BORROWED> bufs
);
bool FileAppend(const char8_t* s, std::span<const BYTE_BUFFER_BORROWED> bufs);

// Appends the given buffer to the end of the given file. Returns `true` on
// success.
static bool FileAppend(const PATH_LITERAL s, const BYTE_BUFFER_BORROWED buf) {
	return FileAppend(s, std::span<const BYTE_BUFFER_BORROWED>{ &buf, 1 });
}
static bool FileAppend(const char8_t* s, const BYTE_BUFFER_BORROWED buf) {
	return FileAppend(s, std::span<const BYTE_BUFFER_BORROWED>{ &buf, 1 });
}

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
	// Tries to fill [buf] with bytes read from the current file position, and
	// returns the number of bytes read. Successful if the returned value is
	// equal to [buf.size_bytes()].
	[[nodiscard]] virtual size_t Read(std::span<uint8_t> buf) = 0;

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
