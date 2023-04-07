/*
 *   2D surfaces via DirectDraw
 *
 */

#include "platform/windows/surface_ddraw.h"

SURFACE_DDRAW::SURFACE_DDRAW()
{
}

SURFACE_DDRAW::~SURFACE_DDRAW()
{
	Delete();
}

void SURFACE_DDRAW::Delete()
{
	if(img) {
		DeleteObject(img);
		img = nullptr;
	}
}
