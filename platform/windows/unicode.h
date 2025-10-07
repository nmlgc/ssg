/*
 *   Unicode types for native system APIs (Win32 Unicode version)
 *
 */

#pragma once

import std;

using UNICODE_STRING = std::wstring;
using UNICODE_LITERAL = const wchar_t *;
#define _UNICODE_INNER(str) L##str
#define _UNICODE(str) _UNICODE_INNER(str)
