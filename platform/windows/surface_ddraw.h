/*
 *   2D surfaces via DirectDraw
 *
 */

#include "platform/buffer.h"
#include "game/surface.h"

struct IDirectDrawSurface;

// Only required for the HBITMAP type, which is basically void*.
#include <windows.h>

struct SURFACE_DDRAW : public SURFACE {
	SURFACE_DDRAW();
	SURFACE_DDRAW(const SURFACE_DDRAW&) = delete;
	SURFACE_DDRAW& operator=(const SURFACE_DDRAW&) = delete;
	~SURFACE_DDRAW();

	// Source bitmap data, if any. Retained for regeneration of the DirectDraw
	// surface if gets lost.
	HBITMAP img = nullptr;

	// Calls Delete() and reinitializes [img] with the contents of [buffer] if
	// they are a valid .BMP.
	bool LoadBMP(BYTE_BUFFER_OWNED buffer);

	void Delete();

	IDirectDrawSurface* surf = nullptr;
	int width;
	int height;
};
