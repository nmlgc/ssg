/*
 *   Common paths and path manipulation
 *
 */

#pragma once

import std;

// Returns the directory containing the game's data. Guaranteed to end with the
// native directory separator. Can be the empty string on platforms with no
// concept of a base data directory.
std::u8string_view PathForData(void);

bool PathIsDirectory(const char8_t *path);

// `char8_t *` wrappers for SDL
// ----------------------------

bool SDL_CreateDirectory(const char8_t *path);
// ----------------------------
