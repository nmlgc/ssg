/*
 *   Unicode types for native system APIs
 *
 */

#pragma once

#ifdef WIN32
#include "platform/windows/unicode.h"
#else
#include "platform/c/unicode.h"
#endif

using PATH_LITERAL = UNICODE_LITERAL;
