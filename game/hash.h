/*
 *   Hash functions and helpers
 *
 */

#pragma once

#include "game/narrow.h"
#include "platform/buffer.h"
#include <blake3.h>

using HASH = std::array<std::byte, BLAKE3_OUT_LEN>;

static constexpr std::optional<HASH> HashFrom(Narrow::string_view str)
{
	if(str.size() != (std::tuple_size_v<HASH> * 2)) {
		return std::nullopt;
	}
	HASH ret = { std::byte{ 0 } };
	uint8_t shift = true;
	for(size_t i = 0; i < str.size(); i++) {
		const auto ch = str[i];
		const auto nibble = (
			((ch >= 'A') && (ch <= 'F')) ? ((ch - 'A') + 0xA) :
			((ch >= 'a') && (ch <= 'f')) ? ((ch - 'a') + 0xA) :
			((ch >= '0') && (ch <= '9')) ? ((ch - '0') + 0x0) :
			0xFF
		);
		if(nibble == 0xFF) {
			return std::nullopt;
		}
		ret[i / 2] |= static_cast<std::byte>(nibble << (shift * 4));
		shift = !shift;
	}
	return ret;
}

// Hashes the given buffer.
static HASH Hash(const BYTE_BUFFER_BORROWED& buffer)
{
	HASH ret;
	blake3_hasher h;
	blake3_hasher_init(&h);
	blake3_hasher_update(&h, buffer.data(), buffer.size_bytes());
	blake3_hasher_finalize(
		&h, reinterpret_cast<uint8_t *>(ret.data()), BLAKE3_OUT_LEN
	);
	return ret;
}

constexpr HASH operator ""_B3(const char* str, size_t len)
{
	const auto ret = HashFrom(Narrow::string_view{ str, len });
	if(!ret) {
		throw "Invalid hash literal";
	}
	return ret.value();
}

// Compile-time-sorted arrays
// --------------------------

template <class T> concept HASHABLE = requires {
	{ T::hash } -> std::same_as<HASH&>;
};

template <HASHABLE T, size_t N> struct SORTED : public std::array<T, N> {
	std::array<T, N>::const_iterator Lookup(const HASH& hash) const noexcept {
		const auto ret = std::ranges::lower_bound(*this, hash, {}, &T::hash);
		if((ret == this->cend()) || (ret->hash != hash)) {
			return this->cend();
		}
		return ret;
	}
};

template <HASHABLE T, size_t N> constexpr SORTED<T, N> HashesSorted(
	std::array<T, N>&& unsorted
) {
	std::ranges::sort(unsorted, [](const auto& a, const auto& b) {
		if(a.hash == b.hash) {
			throw "Duplicated hash";
		}
		return (a.hash < b.hash);
	});
	return SORTED{ unsorted };
}
// --------------------------
