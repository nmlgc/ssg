/*
 *   Memory ownership semantics
 *
 */

#pragma once

import std.compat;

struct BYTE_BUFFER_BORROWED : public std::span<const uint8_t> {
	using span::span;

	template <typename T, size_t N> BYTE_BUFFER_BORROWED(std::span<T, N> val) :
		span(reinterpret_cast<const uint8_t *>(val.data()), val.size_bytes()) {
	}
};

template <
	typename ConstOrNonConstByte
> struct BYTE_BUFFER_CURSOR : public std::span<ConstOrNonConstByte> {
	using std::span<ConstOrNonConstByte>::span;

	template <typename T> using transfer_const = std::conditional_t<
		std::is_const_v<ConstOrNonConstByte>, const T, T
	>;

	size_t cursor = 0;

	// Required to work around a C26495 false positive, for some reason?
	BYTE_BUFFER_CURSOR(const std::span<ConstOrNonConstByte> other) :
		std::span<ConstOrNonConstByte>(other) {
	}

	// Reads up to [n] contiguous values of type T from the active cursor
	// position if possible. If the function returns a valid span, all [n]
	// objects are safe to access.
	template <typename T> std::optional<std::span<transfer_const<T>>> next(
		size_t n = 1
	) {
		const auto cursor_new = (cursor + (sizeof(T) * n));
		if((cursor_new > this->size()) || (cursor_new < cursor)) {
			return std::nullopt;
		}
		#pragma warning(suppress : 26473) // type.1
		auto ret = std::span<transfer_const<T>>{
			reinterpret_cast<transfer_const<T> *>(this->data() + cursor), n
		};
		cursor = cursor_new;
		return ret;
	}
};

// We can't #include SDL headers here because of GCC 15's import-then-#include
// limitations, and SDL_malloc() and SDL_free() are declared with tons of
// attributes we'd rather keep.
void* SDL_malloc_wrap(size_t);

struct SDL_FREE_DELETER {
	void operator()(void *);
};

// Same semantics as the underlying unique_ptr: Can be either allocated or
// empty.
struct BYTE_BUFFER_OWNED : public std::unique_ptr<uint8_t[], SDL_FREE_DELETER> {
private:
	size_t size_;

public:
	// Creates an empty buffer, with no allocation.
	BYTE_BUFFER_OWNED(std::nullptr_t null = nullptr) noexcept :
		std::unique_ptr<uint8_t[], SDL_FREE_DELETER>(null), size_(0) {
	}

	// Tries to allocate [size] bytes, and leaves the buffer empty on failure.
	BYTE_BUFFER_OWNED(size_t size) :
		std::unique_ptr<uint8_t[], SDL_FREE_DELETER>(
			static_cast<uint8_t *>(SDL_malloc_wrap(size))
		),
		size_(get() ? size : 0) {
	}

	auto size() const {
		return size_;
	}

	// Borrows a buffer with an immutable cursor.
	BYTE_BUFFER_CURSOR<const uint8_t> cursor() const {
		return { get(), size() };
	}

	// Borrows a buffer with a mutable cursor.
	BYTE_BUFFER_CURSOR<uint8_t> cursor_mut() {
		return { get(), size() };
	}
};

using BYTE_BUFFER_GROWABLE = std::vector<uint8_t>;
