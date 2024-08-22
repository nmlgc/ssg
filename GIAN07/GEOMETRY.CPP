/*
 *   Backend-independent geometry rendering functions
 *
 */

#include "GEOMETRY.H"
#include "DirectXUTYs/DD_UTY.H"
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

} // namespace Geometry

void GeomCircle(WINDOW_POINT center, PIXEL_COORD radius)
{
	if(DxObj.PixelFormat.IsChanneled()) {
		Geometry::Circle_Approximated(center, radius);
	} else {
		Geometry::Circle_Exact(center, radius);
	}
}
