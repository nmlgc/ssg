/*
 *   File loading
 *
 */

#pragma once

#include "platform/buffer.h"

// Loads the file with the given name into a newly allocated buffer, if
// possible.
BYTE_BUFFER_OWNED FileLoad(
	const char *s, size_t size_limit = (std::numeric_limits<size_t>::max)()
);
