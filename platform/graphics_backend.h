/*
 *   Platform-specific graphics backend interface
 *
 */

#pragma once

#include "game/coords.h"

// Vertex types
// ------------

using VERTEX_COORD = WINDOW_COORD;
using VERTEX_XY = WINDOW_POINT_BASE<VERTEX_COORD>;

template <size_t N = std::dynamic_extent> using VERTEX_XY_SPAN = std::span<
	const VERTEX_XY, N
>;
// ------------
