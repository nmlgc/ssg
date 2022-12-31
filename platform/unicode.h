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

using UNICODE_CODEPOINT = UNICODE_STRING::value_type;

using PATH_STRING = UNICODE_STRING;
using PATH_LITERAL = UNICODE_LITERAL;
using PATH_CODEPOINT = UNICODE_CODEPOINT;
#define _PATH _UNICODE

