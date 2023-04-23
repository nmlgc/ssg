/*
 *   2D surfaces via GDI bitmaps
 *
 */

#pragma once

#include "game/surface.h"
#include "game/format_bmp.h"

// Only required for the HBITMAP type, which is basically void*.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct SURFACE_GDI : public SURFACE {
	// Source bitmap data, if any.
	HBITMAP img = nullptr;

	SURFACE_GDI();
	SURFACE_GDI(const SURFACE_GDI&) = delete;
	SURFACE_GDI& operator=(const SURFACE_GDI&) = delete;
	~SURFACE_GDI();

	// Calls Delete() and reinitializes [img].
	bool Load(BMP_OWNED bmp);

	void Delete();
};
