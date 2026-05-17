/*
 *   2D surfaces via GDI bitmaps
 *
 */

#pragma once

#include "game/surface.h"

// Only required for the HBITMAP type, which is basically void*.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct BMP_OWNED;
struct SDL_IOStream;

class SURFACE_GDI : public SURFACE {
public:
	// Required for unselecting [img] prior to deleting it. Could have probably
	// been `static`, but let's keep it correct for now.
	HGDIOBJ stock_img = nullptr;

	// Source bitmap data, if any.
	HBITMAP img = nullptr;

	// Always has any valid [img] selected into it.
	HDC dc;

	SURFACE_GDI() noexcept;
	SURFACE_GDI(const SURFACE_GDI&) = delete;
	SURFACE_GDI& operator=(const SURFACE_GDI&) = delete;
	~SURFACE_GDI();

	// Calls Delete() and reinitializes [img].
	bool Load(const BMP_OWNED& bmp);

	// Saves [img] as a .BMP file to the given stream.
	bool Save(SDL_IOStream *) const;

	void Delete() noexcept;
};
