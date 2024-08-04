/*
 *   Backend-independent geometry rendering functions
 *
 *   These translate complex geometric shapes to primitives that are better
 *   supported by graphics backends.
 */

#pragma once

#include "platform/graphics_backend.h"

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

void Circle_Approximated(
	GRAPHICS_GEOMETRY_POLY auto& gc, WINDOW_POINT center, PIXEL_COORD radius
)
{
	std::array<VERTEX_XY, CIRCLE_POINTS> xys;
	ApproximateCircle(xys, center, radius);
	gc.DrawLineStrip(xys);
}

// Draw exact points along the outline
void Circle_Exact(
	GRAPHICS_GEOMETRY_FB auto& gf, WINDOW_POINT c, PIXEL_COORD radius
)
{
	if(radius == 0) {
		gf.DrawPoint(c);
		return;
	}

	auto dx = radius;
	auto dy = decltype(dx){ 0 };
	auto s  = radius;

	while(dx >= dy) {
		using _ = WINDOW_POINT;
		gf.DrawPoint(c + _{ +dx, +dy }); gf.DrawPoint(c + _{ +dx, -dy });
		gf.DrawPoint(c + _{ -dx, +dy }); gf.DrawPoint(c + _{ -dx, -dy });
		gf.DrawPoint(c + _{ +dy, +dx }); gf.DrawPoint(c + _{ +dy, -dx });
		gf.DrawPoint(c + _{ -dy, +dx }); gf.DrawPoint(c + _{ -dy, -dx });
		s -= ((dy << 1) + 1);
		if(s < 0) {
			s += ((dx - 1) << 1);
			dx--;
		}
		dy++;
	}
}

void CircleF_Approximated(
	GRAPHICS_GEOMETRY_POLY auto& gp,
	WINDOW_POINT center,
	PIXEL_COORD radius,
	bool alpha
)
{
	std::array<VERTEX_XY, (1 + CIRCLE_POINTS)> xys;
	xys[0].x = static_cast<VERTEX_COORD>(center.x);
	xys[0].y = static_cast<VERTEX_COORD>(center.y);
	ApproximateCircle(
		std::span(xys).subspan<1, CIRCLE_POINTS>(), center, radius
	);
	if(alpha) {
		gp.DrawTrianglesA(TRIANGLE_PRIMITIVE::FAN, xys);
	} else {
		gp.DrawTriangles(TRIANGLE_PRIMITIVE::FAN, xys);
	}
}

void CircleF_Exact(
	GRAPHICS_GEOMETRY_FB auto& gf, WINDOW_POINT center, PIXEL_COORD radius
)
{
	if(radius == 0) {
		gf.DrawPoint(center);
		return;
	}
	auto si = radius;
	auto di = decltype(si){ 0 };
	auto s = radius;
	for(; si >= di; di++) {
		gf.DrawHLine((center.x - si), (center.x + si), (center.y - di));
		gf.DrawHLine((center.x - si), (center.x + si), (center.y + di));
		s -= ((di << 1) + 1);
		if(s >= 0) {
			continue;
		}
		gf.DrawHLine((center.x - di), (center.x + di), (center.y - si));
		gf.DrawHLine((center.x - di), (center.x + di), (center.y + si));
		s += ((--si) << 1);
	}
}

void FatCircleA_Approximated(
	GRAPHICS_GEOMETRY_POLY auto& gp,
	WINDOW_POINT center,
	PIXEL_COORD r,
	PIXEL_COORD w
)
{
	// 普通の円になってしまう場合 //
	if(w >= r) {
		Geometry::CircleF_Approximated(gp, center, (r + w), true);
	}
	std::array<VERTEX_XY, (CIRCLE_POINTS * 2)> xys;
	ApproximateFatCircle(xys, center, r, w);
	gp.DrawTrianglesA(TRIANGLE_PRIMITIVE::STRIP, xys);
}

void GrdRect(
	GRAPHICS_GEOMETRY_POLY auto& gp,
	std::span<const VERTEX_XY, 4> p,
	RGBA col_edge,
	bool alpha
)
{
	const RGBA col_center = { 255, 255, 255, col_edge.a };

	// Use an explicit integer division for a pixel-perfect match of the
	// original look, even if VERTEX_XY is a floating-point type.
	const std::array<VERTEX_XY, 6> xys = {
		p[3],
		p[2],
		(p[0] + p[3]).DivInt(2),
		(p[1] + p[2]).DivInt(2),
		p[0],
		p[1],
	};
	const std::array<VERTEX_RGBA, 6> colors = {
		col_edge, col_edge, col_center, col_center, col_edge, col_edge,
	};
	if(alpha) {
		gp.DrawTrianglesA(TRIANGLE_PRIMITIVE::STRIP, xys, colors);
	} else {
		gp.DrawTriangles(TRIANGLE_PRIMITIVE::STRIP, xys, colors);
	}
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
void GeomCircleFA(
	GRAPHICS_GEOMETRY_POLY auto& gc, WINDOW_POINT center, PIXEL_COORD radius
)
{
	Geometry::CircleF_Approximated(*gc, center, radius, true);
}

// Alpha-blended fat circle
void GeomFatCircleA(
	GRAPHICS_GEOMETRY_POLY auto& gp,
	WINDOW_POINT center,
	PIXEL_COORD r,
	PIXEL_COORD w
)
{
	Geometry::FatCircleA_Approximated(gp, center, r, w);
}

// グラデーション付き長方形(ナナメ可)
void GeomGrdRect(
	GRAPHICS_GEOMETRY_POLY auto& gp,
	std::span<const VERTEX_XY, 4> points,
	RGB col_edge
)
{
	Geometry::GrdRect(gp, points, col_edge.WithAlpha(0xFF), false);
}

// グラデーション付き長方形(ナナメ可+α)
void GeomGrdRectA(
	GRAPHICS_GEOMETRY_POLY auto& gp,
	std::span<const VERTEX_XY, 4> points,
	RGBA col_edge
)
{
	Geometry::GrdRect(gp, points, col_edge, true);
}
// ----------
