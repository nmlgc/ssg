/*
 *   Memory ownership semantics
 *
 */

#pragma once

#include <memory>
#include <optional>
#include <span>
#include <type_traits>
#include <vector>

// We really do not want to accidentally call reinterpret_cast<uint8_t *>() on
// a raw reference to something like an STL type. (Too bad that this doesn't
// catch std::span.)
template <typename T> concept SERIALIZABLE = (
	!std::is_pointer_v<T> && std::is_trivially_copyable_v<T>
);

struct BYTE_BUFFER_BORROWED : public std::span<const uint8_t> {
	using span::span;

	template <SERIALIZABLE T> BYTE_BUFFER_BORROWED(const T& val) :
		span(reinterpret_cast<const uint8_t *>(&val), sizeof(val)) {
	}

	template <typename T> BYTE_BUFFER_BORROWED(std::span<const T> val) :
		span(reinterpret_cast<const uint8_t *>(val.data()), val.size_bytes()) {
	}

	template <typename T> BYTE_BUFFER_BORROWED(std::span<T> val) :
		span(reinterpret_cast<const uint8_t *>(val.data()), val.size_bytes()) {
	}

	template <SERIALIZABLE T> BYTE_BUFFER_BORROWED(const std::vector<T>& val) :
		BYTE_BUFFER_BORROWED(std::span<const T>{ val.data(), val.size() }) {
	}
};

struct BYTE_BUFFER_CURSOR : private BYTE_BUFFER_BORROWED {
	size_t cursor = 0;

	// Reads up to [n] contiguous values of type T from the active cursor
	// position if possible. If the function returns a valid span, all [n]
	// objects are safe to access.
	template <typename T> std::optional<std::span<const T>> next(size_t n = 1) {
		auto cursor_new = (cursor + (sizeof(T) * n));
		if((cursor_new > size()) || (cursor_new < cursor)) {
			return std::nullopt;
		}
		auto ret = std::span<const T>(
			reinterpret_cast<const T *>(data() + cursor), n
		);
		cursor = cursor_new;
		return ret;
	}

	BYTE_BUFFER_CURSOR(BYTE_BUFFER_BORROWED buffer) :
		BYTE_BUFFER_BORROWED(buffer) {
	}
};

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

	// Borrows a buffer with a cursor.
	BYTE_BUFFER_CURSOR cursor() const {
		return { { get(), size() } };
	}
};

using BYTE_BUFFER_GROWABLE = std::vector<uint8_t>;
