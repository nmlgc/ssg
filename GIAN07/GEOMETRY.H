/*
 *   Backend-independent geometry rendering functions
 *
 *   These translate complex geometric shapes to primitives that are better
 *   supported by graphics backends.
 */

#pragma once

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
// ---------------

} // namespace Geometry

// Draw calls
// ----------

// Circle outline
void GeomCircle(WINDOW_POINT center, PIXEL_COORD radius);
// ----------
