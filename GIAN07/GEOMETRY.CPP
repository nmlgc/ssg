/*
 *   Backend-independent geometry rendering functions
 *
 */

#include "GEOMETRY.H"
#include "game/ut_math.h"

namespace Geometry {

constexpr uint8_t CIRCLE_STEP = (0x100 / (CIRCLE_POINTS - 1));

void ApproximateCircle(
	std::span<VERTEX_XY, CIRCLE_POINTS> ret,
	WINDOW_POINT center,
	PIXEL_COORD radius
)
{
	for(auto [i, v] : ret | std::views::enumerate) {
		const uint8_t angle = (i * CIRCLE_STEP);
		v.x = (center.x + cosl(angle, radius));
		v.y = (center.y + sinl(angle, radius));
	}
}

void ApproximateFatCircle(
	std::span<VERTEX_XY, (CIRCLE_POINTS * 2)> ret,
	WINDOW_POINT center,
	PIXEL_COORD r,
	PIXEL_COORD w
)
{
	for(auto [i, v] : ret | std::views::chunk(2) | std::views::enumerate) {
		const uint8_t angle = (i * CIRCLE_STEP);
		const auto [lx, ly] = std::pair(cosl(angle, r), sinl(angle, r));
		const auto [wx, wy] = std::pair(cosl(angle, w), sinl(angle, w));
		v[0] = {
			static_cast<VERTEX_COORD>(center.x + lx - wx),
			static_cast<VERTEX_COORD>(center.y + ly - wy),
		};
		v[1] = {
			static_cast<VERTEX_COORD>(center.x + lx + wx),
			static_cast<VERTEX_COORD>(center.y + ly + wy),
		};
	}
}

} // namespace Geometry

void GeomCircle(WINDOW_POINT center, PIXEL_COORD radius)
{
	if(DxObj.PixelFormat.IsChanneled()) {
		Geometry::Circle_Approximated(center, radius);
	} else {
		Geometry::Circle_Exact(center, radius);
	}
}

void GeomCircleF(WINDOW_POINT center, PIXEL_COORD radius)
{
	if(DxObj.PixelFormat.IsChanneled()) {
		Geometry::CircleF_Approximated(center, radius, false);
	} else {
		Geometry::CircleF_Exact(center, radius);
	}
}
