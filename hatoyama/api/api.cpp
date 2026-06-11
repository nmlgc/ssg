/*
 *   Public API
 *
 */

#include "api/api.h"
#include "api/version.hpp"

const char8_t* VersionBuild(void)
{
	return Version::Build().data();
}

const char8_t* VersionLogic(void)
{
	return Version::Logic().data();
}
