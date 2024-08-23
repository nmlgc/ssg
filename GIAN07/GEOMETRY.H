/*
 *   Backend-independent geometry rendering functions
 *
 *   These translate complex geometric shapes to primitives that are better
 *   supported by graphics backends.
 */

#pragma once

#include "DirectXUTYs/DD_UTY.H"
#include "DirectXUTYs/DD_GRP2D.H"
#include "DirectXUTYs/DD_GRP3D.H"

namespace Geometry {

// Vertex generators
// -----------------

// pbg's Direct3D backend approximated circles as 32-sided polygons. The first
// point is duplicated at the end to simplify index buffer generation.
constexpr size_t CIRCLE_POINTS = 33;

void ApproximateCircle(
	std::span<VERTEX_XY, CIRCLE_POINTS> ret,
	WINDOW_POINT center,
	PIXEL_COORD radius
);

void ApproximateFatCircle(
	std::span<VERTEX_XY, (CIRCLE_POINTS * 2)> ret,
	WINDOW_POINT center,
	PIXEL_COORD r,
	PIXEL_COORD w
);
// -----------------

// Implementations
// ---------------

inline void Circle_Approximated(WINDOW_POINT center, PIXEL_COORD radius)
{
	std::array<WINDOW_POINT, CIRCLE_POINTS> xys;
	ApproximateCircle(xys, center, radius);
	GrpLineStrip(xys);
}

inline void Circle_Exact(WINDOW_POINT center, PIXEL_COORD radius)
{
	// Draw exact points along the outline
	if(radius == 0) {
		GrpPoint(center);
		return;
	}

	auto dx = radius;
	auto dy = decltype(dx){ 0 };
	auto s  = radius;

	while(dx >= dy) {
		using _ = WINDOW_POINT;
		GrpPoint(center + _{ +dx, +dy }); GrpPoint(center + _{ +dx, -dy });
		GrpPoint(center + _{ -dx, +dy }); GrpPoint(center + _{ -dx, -dy });
		GrpPoint(center + _{ +dy, +dx }); GrpPoint(center + _{ +dy, -dx });
		GrpPoint(center + _{ -dy, +dx }); GrpPoint(center + _{ -dy, -dx });
		s -= ((dy << 1) + 1);
		if(s < 0) {
			s += ((dx - 1) << 1);
			dx--;
		}
		dy++;
	}
}

inline void CircleF_Approximated(
	WINDOW_POINT center, PIXEL_COORD radius, bool alpha
)
{
	std::array<VERTEX_XY, (1 + CIRCLE_POINTS)> xys;
	xys[0].x = static_cast<VERTEX_COORD>(center.x);
	xys[0].y = static_cast<VERTEX_COORD>(center.y);
	ApproximateCircle(
		std::span(xys).subspan<1, CIRCLE_POINTS>(), center, radius
	);
	if(alpha) {
		GrpTrianglesA(TRIANGLE_PRIMITIVE::FAN, xys);
	} else {
		GrpTriangles(TRIANGLE_PRIMITIVE::FAN, xys);
	}
}

inline void CircleF_Exact(WINDOW_POINT center, PIXEL_COORD radius)
{
	if(radius == 0) {
		GrpPoint(center);
		return;
	}
	auto si = radius;
	auto di = decltype(si){ 0 };
	auto s = radius;
	for(; si >= di; di++) {
		GrpHLine((center.x - si), (center.x + si), (center.y - di));
		GrpHLine((center.x - si), (center.x + si), (center.y + di));
		s -= ((di << 1) + 1);
		if(s >= 0) {
			continue;
		}
		GrpHLine((center.x - di), (center.x + di), (center.y - si));
		GrpHLine((center.x - di), (center.x + di), (center.y + si));
		s += ((--si) << 1);
	}
}

inline void FatCircleA_Approximated(
	WINDOW_POINT center, PIXEL_COORD r, PIXEL_COORD w
)
{
	// 普通の円になってしまう場合 //
	if(w >= r) {
		CircleF_Approximated(center, (r + w), true);
	}
	std::array<VERTEX_XY, (CIRCLE_POINTS * 2)> xys;
	ApproximateFatCircle(xys, center, r, w);
	GrpTrianglesA(TRIANGLE_PRIMITIVE::STRIP, xys);
}
// ---------------

} // namespace Geometry

// Draw calls
// ----------

// Circle outline
void GeomCircle(WINDOW_POINT center, PIXEL_COORD radius);

// Filled circle
void GeomCircleF(WINDOW_POINT center, PIXEL_COORD radius);

// Filled and alpha-blended circle
inline void GeomCircleFA(WINDOW_POINT center, PIXEL_COORD radius)
{
	assert(DxObj.PixelFormat.IsChanneled());
	Geometry::CircleF_Approximated(center, radius, true);
}

// Alpha-blended fat circle
inline void GeomFatCircleA(WINDOW_POINT center, PIXEL_COORD r, PIXEL_COORD w)
{
	assert(DxObj.PixelFormat.IsChanneled());
	Geometry::FatCircleA_Approximated(center, r, w);
}
// ----------
