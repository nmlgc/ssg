/*
 *   2D surfaces via DirectDraw
 *
 */

#include "platform/buffer.h"
#include "game/surface.h"

struct IDirectDrawSurface;

struct SURFACE_DDRAW : public SURFACE {
	// Source bitmap data, if any. Retained for regeneration of the DirectDraw
	// surface if gets lost.
	BYTE_BUFFER_OWNED img;

	IDirectDrawSurface* surf = nullptr;
	int width;
	int height;
};
