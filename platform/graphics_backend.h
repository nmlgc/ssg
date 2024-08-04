/*
 *   Platform-specific graphics backend interface
 *
 */

#pragma once

#include "game/coords.h"
#include "game/graphics.h"
#include "game/narrow.h"
#include "game/pixelformat.h"

/// Enumeration and pre-initialization queries
/// ------------------------------------------

// Physical graphics adapters.
uint8_t GrpBackend_DeviceCount(void);
Any::string_view GrpBackend_DeviceName(uint8_t id);
/// ------------------------------------------

/// General
/// -------

// Clears the backbuffer with the given palettized or channeled color,
// depending on the mode.
void GrpBackend_Clear(
	uint8_t col_palettized = RGB216{ 0, 0, 0 }.PaletteIndex(),
	RGB col_channeled = RGB{ 0, 0, 0 }
);

// Returns the current pixel format of the backbuffer.
PIXELFORMAT GrpBackend_PixelFormat(void);

struct FILE_STREAM_WRITE;
void GrpBackend_Flip(std::unique_ptr<FILE_STREAM_WRITE> screenshot_stream);
/// -------

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

// Interface for geometry draw calls that require true-color polygon rendering.
// The [colors] span either must be either
// • empty or omitted (which will render all vertices using the last SetColor()
//   and SetAlpha*() value), or
// • have as many elements as [points].
template <class T> concept GRAPHICS_GEOMETRY_POLY = requires(
	T t,
	WINDOW_COORD coord,
	TRIANGLE_PRIMITIVE tp,
	VERTEX_XY_SPAN<> points,
	VERTEX_RGBA_SPAN<> colors,
	RGB rgb
) {
	t.DrawLineStrip(points);
	t.DrawTriangles(tp, points, colors);
	t.DrawTrianglesA(tp, points, colors);

	// スペアな用グラデーションライン
	t.DrawGrdLineEx(coord, coord, rgb, coord, rgb);
};

// Interface for framebuffer-exclusive geometry draw calls.
template <class T> concept GRAPHICS_GEOMETRY_FB = requires(
	T t, WINDOW_COORD coord, WINDOW_POINT p
) {
	t.DrawPoint(p);
	t.DrawHLine(coord, coord, coord);
};
/// --------

#ifdef WIN32
	#include "DirectXUTYs/DD_UTY.H"
#endif
