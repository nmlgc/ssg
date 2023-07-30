/*
 *   Coordinate systems
 *
 */

#include "coords.h"

PIXEL_POINT& operator -=(PIXEL_POINT& self, const PIXEL_SIZE& other) {
	self.x -= other.w;
	self.y -= other.h;
	return self;
}
