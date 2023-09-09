/*
 *   File loading
 *
 */

#pragma once

#include "platform/buffer.h"
#include "platform/unicode.h"

// Fills [buf] with the first [buf.size_bytes()] read from the file with the
// given name, and returns the number of bytes read. Successful if the returned
// value is equal to [buf.size_bytes()].
[[nodiscard]] size_t FileLoadInplace(
	std::span<uint8_t> buf, const PATH_LITERAL s
);

// Loads an instance of the given type from the first sizeof(T) bytes of the
// given file, if possible.
template <SERIALIZABLE T> std::optional<T> FileLoad(const PATH_LITERAL s) {
	T ret;
	if(FileLoadInplace(
		{ reinterpret_cast<uint8_t *>(&ret), sizeof(ret) }, s
	) != sizeof(T)) {
		return std::nullopt;
	}
	return ret;
}

// Loads the file with the given name into a newly allocated buffer, if
// possible.
BYTE_BUFFER_OWNED FileLoad(
	const PATH_LITERAL s,
	size_t size_limit = (std::numeric_limits<size_t>::max)()
);

// Saves the given sequence of buffers to the file with the given name,
// overwriting the file if it already exists. Returns `true` on success.
bool FileWrite(
	const PATH_LITERAL s, std::span<const BYTE_BUFFER_BORROWED> bufs
);

// Saves the given buffer to the file with the given name, overwriting the file
// if it already exists. Returns `true` on success.
static bool FileWrite(const PATH_LITERAL s, const BYTE_BUFFER_BORROWED buf) {
	return FileWrite(s, std::span<const BYTE_BUFFER_BORROWED>{ &buf, 1 });
}

// Appends the given sequence of buffers to the end of the given file. Returns
// `true` on success.
bool FileAppend(
	const PATH_LITERAL s, std::span<const BYTE_BUFFER_BORROWED> bufs
);

// Appends the given buffer to the end of the given file. Returns `true` on
// success.
static bool FileAppend(const PATH_LITERAL s, const BYTE_BUFFER_BORROWED buf) {
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

struct FILE_STREAM_WRITE : FILE_STREAM_SEEK {
	// Retuns `true` if the buffer was written successfully.
	[[nodiscard]] virtual bool Write(BYTE_BUFFER_BORROWED buf) = 0;
};

std::unique_ptr<FILE_STREAM_WRITE> FileStreamWrite(const PATH_LITERAL s);
// -------
