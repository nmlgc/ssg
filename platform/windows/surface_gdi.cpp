/*
 *   2D surfaces via GDI bitmaps
 *
 */

#include "platform/windows/surface_gdi.h"

SURFACE_GDI::SURFACE_GDI() :
	dc(CreateCompatibleDC(GetDC(nullptr)))
{
}

SURFACE_GDI::~SURFACE_GDI()
{
	Delete();
	DeleteDC(dc);
}

void SURFACE_GDI::Delete()
{
	if(img) {
		SelectObject(dc, stock_img);
		DeleteObject(img);
		img = nullptr;
	}
}
