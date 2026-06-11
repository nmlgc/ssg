/*
 *   Replay functions
 *
 */

#include "api/version.hpp"
#include "logic_version.h"
#include "obj/version.h"

namespace Version {
std::u8string_view Build(void)
{
	return reinterpret_cast<const char8_t *>(VERSION_TAG);
}

std::u8string_view Logic(void)
{
	return reinterpret_cast<const char8_t *>(LOGIC_VERSION);
}
}
