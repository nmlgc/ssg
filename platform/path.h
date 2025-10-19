/*
 *   Common paths
 *
 */

#pragma once

import std;

// Returns the directory containing the game's data. Guaranteed to end with the
// native directory separator. Can be the empty string on platforms with no
// concept of a base data directory.
std::u8string_view PathForData(void);

bool PathIsDirectory(const char8_t *path);
