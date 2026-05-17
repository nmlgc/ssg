/*
 *   Hash function
 *
 */

// GCC 15 throws `error: conflicting declaration 'typedef struct max_align_t
// max_align_t'` if this appears after a module import.
#include <blake3.h>

#include "game/hash.h"

static_assert(
	(sizeof(HASH) == BLAKE3_OUT_LEN),
	"hardcoded hash size in header must match algorithm's output size"
);

HASH Hash(const BYTE_BUFFER_BORROWED& buffer)
{
	HASH ret;
	blake3_hasher h;
	blake3_hasher_init(&h);
	blake3_hasher_update(&h, buffer.data(), buffer.size_bytes());
	blake3_hasher_finalize(
		&h, std::bit_cast<uint8_t *>(ret.data()), BLAKE3_OUT_LEN
	);
	return ret;
}
