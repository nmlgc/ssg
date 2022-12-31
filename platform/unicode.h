/*
 *   Unicode types for native system APIs
 *
 */

#pragma once

	#include "platform/c/unicode.h"

using UNICODE_CODEPOINT = UNICODE_STRING::value_type;

using PATH_STRING = UNICODE_STRING;
using PATH_LITERAL = UNICODE_LITERAL;
using PATH_CODEPOINT = UNICODE_CODEPOINT;
#define _PATH _UNICODE

