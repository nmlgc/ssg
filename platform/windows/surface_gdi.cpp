/*
 *   2D surfaces via GDI bitmaps
 *
 */

#include "platform/windows/surface_gdi.h"

SURFACE_GDI::SURFACE_GDI()
{
}

SURFACE_GDI::~SURFACE_GDI()
{
	Delete();
}

void SURFACE_GDI::Delete()
{
	if(img) {
		DeleteObject(img);
		img = nullptr;
	}
}
