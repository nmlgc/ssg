/*
 *   File loading
 *
 */

#pragma once

#include "platform/buffer.h"

// Fills [buf] with the first [buf.size_bytes()] read from the file with the
// given name, and returns the number of bytes read. Successful if the returned
// value is equal to [buf.size_bytes()].
[[nodiscard]] size_t FileLoadInplace(std::span<uint8_t> buf, const char *s);

// Loads an instance of the given type from the first sizeof(T) bytes of the
// given file, if possible.
template <SERIALIZABLE T> std::optional<T> FileLoad(const char *s) {
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
	const char *s, size_t size_limit = (std::numeric_limits<size_t>::max)()
);

// Saves the given sequence of buffers to the file with the given name,
// overwriting the file if it already exists. Returns `true` on success.
bool FileWrite(const char *s, std::span<const BYTE_BUFFER_BORROWED> bufs);

// Saves the given buffer to the file with the given name, overwriting the file
// if it already exists. Returns `true` on success.
static bool FileWrite(const char *s, const BYTE_BUFFER_BORROWED buf) {
	return FileWrite(s, std::span<const BYTE_BUFFER_BORROWED>{ &buf, 1 });
}
