/*
 *   Unicode types for native system APIs (generic narrow/bag-of-bytes version)
 *
 */

#pragma once

import std;

using UNICODE_STRING = std::string;
using UNICODE_STRING_VIEW = std::string_view;
using UNICODE_LITERAL = const char *;
#define _UNICODE(str) str
