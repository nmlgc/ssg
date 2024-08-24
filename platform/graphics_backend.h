/*
 *   Platform-specific graphics backend interface
 *
 */

#pragma once

#include "game/coords.h"
#include "game/graphics.h"

// Vertex types
// ------------

using VERTEX_COORD = WINDOW_COORD;
using VERTEX_XY = WINDOW_POINT_BASE<VERTEX_COORD>;
using VERTEX_RGBA = RGBA;

template <size_t N = std::dynamic_extent> using VERTEX_XY_SPAN = std::span<
	const VERTEX_XY, N
>;
template <size_t N = std::dynamic_extent> using VERTEX_RGBA_SPAN = std::span<
	const VERTEX_RGBA, N
>;

enum class TRIANGLE_PRIMITIVE : uint8_t {
	FAN,
	STRIP,
	COUNT
};
// ------------
