/*
 *   Unicode types for native system APIs (generic narrow/bag-of-bytes version)
 *
 */

#pragma once

import std;

using UNICODE_STRING = std::string;
using UNICODE_LITERAL = const char *;
#define _UNICODE(str) str
