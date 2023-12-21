/*
 *   Hash functions and helpers
 *
 */

#pragma once

#include "platform/buffer.h"
#include <blake3.h>
#include <array>

using HASH = std::array<std::byte, BLAKE3_OUT_LEN>;

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
