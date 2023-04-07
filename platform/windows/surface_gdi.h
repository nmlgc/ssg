/*
 *   2D surfaces via GDI bitmaps
 *
 */

#pragma once

#include "game/surface.h"
#include "platform/buffer.h"

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

	// Calls Delete() and reinitializes [img] with the contents of [buffer] if
	// they are a valid .BMP.
	bool LoadBMP(BYTE_BUFFER_OWNED buffer);

	void Delete();
};
