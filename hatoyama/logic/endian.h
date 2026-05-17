/*
 *   Little- and big-endian integer types
 *
 *   Why did the C++ committee standardize std::endian, but not anything like
 *   this?!
 *   (See https://commandcenter.blogspot.com/2012/04/byte-order-fallacy.html.)
 */

#pragma once

import std.compat;

template <typename T, bool Big> class ENDIAN_VALUE {
	std::byte v[sizeof(T)];

	using UT = std::make_unsigned_t<T>;
	static constexpr UT ShiftOffset(uint8_t byte) {
		return ((Big ? (sizeof(T) - 1 - byte) : byte) * 8);
	}

public:
	ENDIAN_VALUE() noexcept {
		*this = 0;
	}

	ENDIAN_VALUE(const T& other) noexcept {
		*this = other;
	}

	constexpr operator T() const noexcept {
		T ret = 0;
		for(uint8_t byte = 0; byte < sizeof(T); byte++) {
			ret |= (std::to_integer<const UT>(v[byte]) << ShiftOffset(byte));
		}
		return ret;
	}

	const T& operator =(const T& other) noexcept {
		for(uint8_t byte = 0; byte < sizeof(T); byte++) {
			v[byte] = static_cast<std::byte>(
				static_cast<UT>(other) >> ShiftOffset(byte)
			);
		}
		return other;
	}
};

template <typename T> using ENDIAN_LITTLE = ENDIAN_VALUE<T, false>;
template <typename T> using ENDIAN_BIG = ENDIAN_VALUE<T, true>;

// Let's help the optimizer a bit.
template <typename T> using ENDIAN_SELECT_LITTLE = std::conditional_t<
	(std::endian::native == std::endian::little), T, ENDIAN_LITTLE<T>
>;
template <typename T> using ENDIAN_SELECT_BIG = std::conditional_t<
	(std::endian::native == std::endian::big), T, ENDIAN_BIG<T>
>;

using I16LE = ENDIAN_SELECT_LITTLE<int16_t>;
using U16LE = ENDIAN_SELECT_LITTLE<uint16_t>;
using I32LE = ENDIAN_SELECT_LITTLE<int32_t>;
using U32LE = ENDIAN_SELECT_LITTLE<uint32_t>;
using I16BE = ENDIAN_SELECT_BIG<int16_t>;
using U16BE = ENDIAN_SELECT_BIG<uint16_t>;
using I32BE = ENDIAN_SELECT_BIG<int32_t>;
using U32BE = ENDIAN_SELECT_BIG<uint32_t>;

static I16LE I16LEAt(const void *p) { return *static_cast<const I16LE *>(p); }
static U16LE U16LEAt(const void *p) { return *static_cast<const U16LE *>(p); }
static I32LE I32LEAt(const void *p) { return *static_cast<const I32LE *>(p); }
static U32LE U32LEAt(const void *p) { return *static_cast<const U32LE *>(p); }
static I16BE I16BEAt(const void *p) { return *static_cast<const I16BE *>(p); }
static U16BE U16BEAt(const void *p) { return *static_cast<const U16BE *>(p); }
static I32BE I32BEAt(const void *p) { return *static_cast<const I32BE *>(p); }
static U32BE U32BEAt(const void *p) { return *static_cast<const U32BE *>(p); }
