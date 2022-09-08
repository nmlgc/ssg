/*
 *   File loading
 *
 */

#pragma once

#include <optional>
#include "platform/buffer.h"

// Loads the file with the given name into a newly allocated buffer, if
// possible.
std::optional<BYTE_BUFFER_OWNED> FileLoad(
	const char *s, size_t size_limit = (std::numeric_limits<size_t>::max)()
);
