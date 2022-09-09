/*
 *   Memory ownership semantics
 *
 */

#pragma once

#include <memory>
#include <span>
#include <vector>

using BYTE_BUFFER_BORROWED = std::span<const uint8_t>;

// Same semantics as the underlying unique_ptr: Can be either allocated or
// empty.
struct BYTE_BUFFER_OWNED : public std::unique_ptr<uint8_t[]> {
private:
	size_t size_;

public:
	// Creates an empty buffer, with no allocation.
	BYTE_BUFFER_OWNED(std::nullptr_t null = nullptr) :
		std::unique_ptr<uint8_t[]>(null),
		size_(0) {
	}

	// Tries to allocate [size] bytes, and leaves the buffer empty on failure.
	BYTE_BUFFER_OWNED(size_t size) :
		std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[size]),
		size_(!this ? 0 : size) {
	}

	auto size() const {
		return size_;
	}
};

using BYTE_BUFFER_GROWABLE = std::vector<uint8_t>;
