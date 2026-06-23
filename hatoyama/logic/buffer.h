/*
 *   Memory ownership semantics
 *
 */

#pragma once

#include <assert.h>
import std.compat;

struct BUFFER_BORROWED : public std::span<const uint8_t> {
	using span::span;

	template <size_t N> BUFFER_BORROWED(std::span<const uint8_t, N> val) :
		span(val.data(), val.size_bytes()) {
	}

	template <typename T, size_t N> BUFFER_BORROWED(std::span<T, N> val) :
		span(reinterpret_cast<const uint8_t *>(val.data()), val.size_bytes()) {
	}
};

template <
	typename ConstOrNonConstByte
> struct BUFFER_CURSOR : public std::span<ConstOrNonConstByte> {
	using std::span<ConstOrNonConstByte>::span;

	template <typename T> using transfer_const = std::conditional_t<
		std::is_const_v<ConstOrNonConstByte>, const T, T
	>;

	size_t cursor = 0;

	// Required to work around a C26495 false positive, for some reason?
	BUFFER_CURSOR(const std::span<ConstOrNonConstByte> other) :
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

// Defines a heap for use with `BUFFER_OWNED`.
struct BUFFER_HEAP {
	void* (*allocate)(size_t size) noexcept;
	void (*free)(void *buf) noexcept;
};

// And since pointers to structs can't be callable… This indeed creates smaller
// code than just using the [free] pointer itself as the template parameter of
// `BUFFER_OWNED`.
struct BUFFER_DELETER {
	void (*free)(void *buf) noexcept;

	void operator()(void *buf) const {
		// std::unique_ptr will only ever call this for non-`nullptr` pointers.
		// If [free] is a `nullptr`, that's on us.
		assert(free != nullptr);
		free(buf);
	}
};

// Low-dependency heap for game logic allocations
extern const BUFFER_HEAP BUFFER_HEAP_LOGIC;

// Same semantics as the underlying unique_ptr: Can be either allocated or
// empty.
struct BUFFER_OWNED : public std::unique_ptr<uint8_t[], BUFFER_DELETER> {
private:
	size_t size_;

public:
	// Creates an empty buffer, with no allocation.
	BUFFER_OWNED(std::nullptr_t null = nullptr) noexcept :
		std::unique_ptr<uint8_t[], BUFFER_DELETER>(nullptr, { nullptr }),
		size_(0) {
	}

	// Adopts a pre-allocated buffer from the given heap.
	BUFFER_OWNED(void *&& buf, size_t size, const BUFFER_HEAP& heap) :
		std::unique_ptr<uint8_t[], BUFFER_DELETER>(
			static_cast<uint8_t *>(buf), { heap.free }
		),
		size_(size)
	{
	}

	// Tries to allocate [size] bytes, and leaves the buffer empty on failure.
	BUFFER_OWNED(size_t size, const BUFFER_HEAP& heap) :
		std::unique_ptr<uint8_t[], BUFFER_DELETER>(
			static_cast<uint8_t *>(heap.allocate(size)), { heap.free }
		),
		size_(get() ? size : 0) {
	}

	auto size() const {
		return size_;
	}

	explicit operator BUFFER_BORROWED() const {
		return { get(), size() };
	}

	// Borrows a buffer with an immutable cursor.
	BUFFER_CURSOR<const uint8_t> cursor() const {
		return { get(), size() };
	}

	// Borrows a buffer with a mutable cursor.
	BUFFER_CURSOR<uint8_t> cursor_mut() {
		return { get(), size() };
	}
};

using BUFFER_GROWABLE = std::vector<uint8_t>;
