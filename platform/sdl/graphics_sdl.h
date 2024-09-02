/*
 *   Graphics via SDL_Renderer
 *
 */

#pragma once

#include "platform/graphics_backend.h"

class GRAPHICS_GEOMETRY_SDL : public GRAPHICS_GEOMETRY {
public:
	// Generic methods
	// ---------------

	void Lock(void) final;
	void Unlock(void) final;
	void SetColor(RGB216 col) final;
	void SetAlphaNorm(uint8_t a) final;
	void SetAlphaOne(void) final;
	void DrawLine(int x1, int y1, int x2, int y2) final;
	void DrawBox(int x1, int y1, int x2, int y2) final;
	void DrawBoxA(int x1, int y1, int x2, int y2) final;
	void DrawTriangleFan(VERTEX_XY_SPAN<>) final;
	// ----------

	// Poly methods
	// ------------

	void DrawLineStrip(VERTEX_XY_SPAN<>);
	void DrawTriangles(
		TRIANGLE_PRIMITIVE, VERTEX_XY_SPAN<>, VERTEX_RGBA_SPAN<> colors = {}
	);
	void DrawTrianglesA(
		TRIANGLE_PRIMITIVE, VERTEX_XY_SPAN<>, VERTEX_RGBA_SPAN<> colors = {}
	);
	void DrawGrdLineEx(int x, int y1, RGB c1, int y2, RGB c2);
	// ------------

	// Framebuffer methods
	// -------------------
	// Just required to satisfy the framebuffer concept, since our GrpGeom_FB()
	// also returns a pointer to this class.

	void DrawPoint(WINDOW_POINT p);
	void DrawHLine(int x1, int x2, int y);
	// -------------------
};

extern GRAPHICS_GEOMETRY_SDL GrpGeomSDL;
#define GrpGeom (&GrpGeomSDL)

GRAPHICS_GEOMETRY_SDL *GrpGeom_Poly(void);
GRAPHICS_GEOMETRY_SDL *GrpGeom_FB(void);

// Must be kept in sync with the hardcoded ones in the SDL_GL_ResetAttributes()
// implementation.
#define OPENGL_TARGET_CORE_MAJ 2
#define OPENGL_TARGET_CORE_MIN 1
#define OPENGL_TARGET_ES1_MIN 1
#define OPENGL_TARGET_ES2_MIN 0

#undef GRP_SUPPORT_BITDEPTH
#define GRP_SUPPORT_SCALING
#define GRP_SUPPORT_WINDOWED
#define GRP_SUPPORT_API
