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

class SURFACE_GDI : public SURFACE {
public:
	// Required for unselecting [img] prior to deleting it. Could have probably
	// been `static`, but let's keep it correct for now.
	HGDIOBJ stock_img;

	// Source bitmap data, if any.
	HBITMAP img = nullptr;

	// Always has any valid [img] selected into it.
	HDC dc;

	SURFACE_GDI();
	SURFACE_GDI(const SURFACE_GDI&) = delete;
	SURFACE_GDI& operator=(const SURFACE_GDI&) = delete;
	~SURFACE_GDI();

	// Calls Delete() and reinitializes [img].
	bool Load(BMP_OWNED bmp);

	// Saves [img] to a .BMP file with the given name.
	bool Save(const PATH_LITERAL s) const;

	void Delete();
};
