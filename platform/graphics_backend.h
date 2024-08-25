/*
 *   Platform-specific graphics backend interface
 *
 */

#pragma once

import std.compat;
#include "game/coords.h"
#include "game/graphics.h"

/// Geometry
/// --------

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

// Base interface for geometry draw calls that can be implemented differently
// for channeled and palettized pixel modes. Implementations can decide whether
// to use this interface
// • polymorphically and have [GrpGeom] be a GRAPHICS_GEOMETRY* that points to
//   the channeled or palettized subclass, or
// • just as a concept to constrain a single shared renderer subclass, and then
//   have [GrpGeom] be an instance of that single subclass.
class GRAPHICS_GEOMETRY {
public:
	// Rendering state
	// ---------------
	// SetColor() should only affect Draw*() calls, and SetAlpha() should only
	// affect Draw*A() calls. Backends with equally stateful handling of alpha
	// blending must implement this as follows:
	//
	// • Leave alpha blending deactivated for all non-*A() calls. If the
	//   backend requires RGBA color values for vertices of those calls as
	//   well, set their alpha component to 0xFF.
	// • Selectively activate alpha blending only during *A() calls and
	//   immediately disable it before returning.

	virtual void Lock(void) = 0;	// 図形描画の準備をする
	virtual void Unlock(void) = 0;	// 図形描画を完了する

	virtual void SetColor(RGB216 col) = 0;	// 色セット

	// Enables regular alpha blending.
	// dstRGB = (srcRGB * [a]) + (dstRGB * (1 - [a]))
	virtual void SetAlphaNorm(uint8_t a) = 0;

	// Enables additive blending with a fixed alpha factor of 1.
	// dstRGB = (srcRGB * 1) + dstRGB
	virtual void SetAlphaOne(void) = 0;
	// ---------------

	// Draw calls
	// ----------

	// 直線
	virtual void DrawLine(int x1, int y1, int x2, int y2) = 0;

	// 長方形
	virtual void DrawBox(int x1, int y1, int x2, int y2) = 0;

	// α長方形
	virtual void DrawBoxA(int x1, int y1, int x2, int y2) = 0;

	virtual void DrawTriangleFan(VERTEX_XY_SPAN<>) = 0;
	// ----------

	virtual ~GRAPHICS_GEOMETRY() {
	}
};
/// --------

#ifdef WIN32
	#include "DirectXUTYs/DD_UTY.H"
#endif
