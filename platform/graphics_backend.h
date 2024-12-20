/*
 *   Platform-specific graphics backend interface
 *
 */

#pragma once

#include "constants.h"
#include "game/graphics.h"
#include "game/narrow.h"

/// Enumeration and pre-initialization queries
/// ------------------------------------------

// Should initialize everything needed for device and API queries.
bool GrpBackend_Enum(void);

// Physical graphics adapters.
uint8_t GrpBackend_DeviceCount(void);
Any::string_view GrpBackend_DeviceName(uint8_t id);

// Rendering APIs.
int8_t GrpBackend_APICount(void);
std::u8string_view GrpBackend_APIName(int8_t id);

// Returns the maximum usable display size in windowed or fullscreen mode.
PIXEL_SIZE GrpBackend_DisplaySize(bool fullscreen);
/// ------------------------------------------

/// Initialization and cleanup
/// --------------------------

// Tries initializing the backend with the closest available configuration for
// the given [params], which can be assumed to be valid. Called after
// GrpBackend_Enum(). Returns the actual configuration the backend is running
// and whether the call site must reinitialize any surfaces, or `std::nullopt`
// if no valid format could be found.
//
// If [maybe_prev] is valid, this call is supposed to reinitialize an already
// running backend with new parameters.
std::optional<GRAPHICS_INIT_RESULT> GrpBackend_Init(
	std::optional<const GRAPHICS_PARAMS> maybe_prev, GRAPHICS_PARAMS params
);

// いつも通りに(ただし失敗したら異常終了)
void GrpBackend_Cleanup(void);
/// --------------------------

/// General
/// -------

// Clears the backbuffer with the given palettized or channeled color,
// depending on the mode.
void GrpBackend_Clear(
	uint8_t col_palettized = RGB216{ 0, 0, 0 }.PaletteIndex(),
	RGB col_channeled = RGB{ 0, 0, 0 }
);

// Sets the clipping rectangle.
void GrpBackend_SetClip(const WINDOW_LTRB& rect);

// Returns the current pixel format of the backbuffer.
PIXELFORMAT GrpBackend_PixelFormat(void);

// Retrieves the current backbuffer palette. Does nothing in channeled mode.
void GrpBackend_PaletteGet(PALETTE& pal);

// Sets the current backbuffer palette. Does nothing in channeled mode.
bool GrpBackend_PaletteSet(const PALETTE& pal);

struct FILE_STREAM_WRITE;
void GrpBackend_Flip(std::unique_ptr<FILE_STREAM_WRITE> screenshot_stream);
/// -------

/// Surfaces
/// --------
struct BMP_OWNED;

// (Re-)creates the texture in the given surface slot with the given size,
// using the format indicated by GrpBackend_PixelFormat(), and with undefined
// initial contents.
bool GrpSurface_CreateUninitialized(SURFACE_ID sid, const PIXEL_SIZE& size);

// Consumes the given .BMP file and sets the given surface to its contents,
// re-creating it in the correct size if necessary.
bool GrpSurface_Load(SURFACE_ID sid, BMP_OWNED&& bmp);

bool GrpSurface_PaletteApplyToBackend(SURFACE_ID sid);

// Uploads [pixels] (consisting of a pointer and a row pitch) to a [subrect] of
// [sid]. [subrect] can be a `nullptr` to overwrite the entire texture. The
// [pixels] have to use the format indicated by GrpBackend_PixelFormat().
bool GrpSurface_Update(
	SURFACE_ID sid,
	const PIXEL_LTWH* subrect,
	std::tuple<const std::byte *, size_t> pixels
) noexcept;

// Returns the size of the given surface.
PIXEL_SIZE GrpSurface_Size(SURFACE_ID sid);

// Blits the given [src] rectangle inside [sid] to the given top-left point
// on the backbuffer while clipping the destination rectangle to the clipping
// area. Returns `true` if any part of the sprite was blitted.
bool GrpSurface_Blit(
	WINDOW_POINT topleft, SURFACE_ID sid, const PIXEL_LTRB& src
);

// Like GrpSurface_Blit(), but ignores [sid]'s color key.
void GrpSurface_BlitOpaque(
	WINDOW_POINT topleft, SURFACE_ID sid, const PIXEL_LTRB& src
);

#ifdef WIN32
	// Win32 GDI text rendering bridge
	// -------------------------------
	class SURFACE_GDI;

	// Returns a reference to the backend's designated GDI text surface.
	SURFACE_GDI& GrpSurface_GDIText_Surface(void) noexcept;

	// (Re-)creates the backend's designated GDI text surface with the given
	// size and undefined initial contents. [w] and [h] have already been
	// validated to fit into the positive range of a signed 32-bit integer.
	// After filling it with the intended pixels, call
	// GrpSurface_GDIText_Update() to upload them to the backend.
	bool GrpSurface_GDIText_Create(int32_t w, int32_t h, RGB colorkey);

	bool GrpSurface_GDIText_Update(const PIXEL_LTWH& r);
	// -------------------------------
#endif
/// --------

/// Geometry
/// --------

// Vertex types
// ------------

#ifdef WIN32_VINTAGE
	using VERTEX_COORD = WINDOW_COORD;
#else
	using VERTEX_COORD = float;
#endif
using VERTEX_XY = WINDOW_POINT_BASE<VERTEX_COORD>;
using VERTEX_RGBA = RGBA;

template <size_t N = std::dynamic_extent> using VERTEX_XY_SPAN = std::span<
	const VERTEX_XY, N
>;
template <size_t N = std::dynamic_extent> using VERTEX_RGBA_SPAN = std::span<
	const VERTEX_RGBA, N
>;

enum class TRIANGLE_PRIMITIVE : uint8_t {
	FAN,
	STRIP,
	COUNT
};
// ------------

// Base interface for geometry draw calls that can be implemented differently
// for channeled and palettized pixel modes. Implementations can decide whether
// to use this interface
// • polymorphically and have [GrpGeom] be a GRAPHICS_GEOMETRY* that points to
//   the channeled or palettized subclass, or
// • just as a concept to constrain a single shared renderer subclass, and then
//   have [GrpGeom] be an instance of that single subclass.
class GRAPHICS_GEOMETRY {
public:
	// Rendering state
	// ---------------
	// SetColor() should only affect Draw*() calls, and SetAlpha() should only
	// affect Draw*A() calls. Backends with equally stateful handling of alpha
	// blending must implement this as follows:
	//
	// • Leave alpha blending deactivated for all non-*A() calls. If the
	//   backend requires RGBA color values for vertices of those calls as
	//   well, set their alpha component to 0xFF.
	// • Selectively activate alpha blending only during *A() calls and
	//   immediately disable it before returning.

	virtual void Lock(void) = 0;	// 図形描画の準備をする
	virtual void Unlock(void) = 0;	// 図形描画を完了する

	virtual void SetColor(RGB216 col) = 0;	// 色セット

	// Enables regular alpha blending.
	// dstRGB = (srcRGB * [a]) + (dstRGB * (1 - [a]))
	virtual void SetAlphaNorm(uint8_t a) = 0;

	// Enables additive blending with a fixed alpha factor of 1.
	// dstRGB = (srcRGB * 1) + dstRGB
	virtual void SetAlphaOne(void) = 0;
	// ---------------

	// Draw calls
	// ----------

	// 直線
	virtual void DrawLine(int x1, int y1, int x2, int y2) = 0;

	// 長方形
	virtual void DrawBox(int x1, int y1, int x2, int y2) = 0;

	// α長方形
	virtual void DrawBoxA(int x1, int y1, int x2, int y2) = 0;

	virtual void DrawTriangleFan(VERTEX_XY_SPAN<>) = 0;
	// ----------

	virtual ~GRAPHICS_GEOMETRY() {
	}
};

// Interface for geometry draw calls that require true-color polygon rendering.
// The [colors] span either must be either
// • empty or omitted (which will render all vertices using the last SetColor()
//   and SetAlpha*() value), or
// • have as many elements as [points].
template <class T> concept GRAPHICS_GEOMETRY_POLY = requires(
	T t,
	WINDOW_COORD coord,
	TRIANGLE_PRIMITIVE tp,
	VERTEX_XY_SPAN<> points,
	VERTEX_RGBA_SPAN<> colors,
	RGB rgb
) {
	t.DrawLineStrip(points);
	t.DrawTriangles(tp, points, colors);
	t.DrawTrianglesA(tp, points, colors);

	// スペアな用グラデーションライン
	t.DrawGrdLineEx(coord, coord, rgb, coord, rgb);
};

// Interface for framebuffer-exclusive geometry draw calls.
template <class T> concept GRAPHICS_GEOMETRY_FB = requires(
	T t, WINDOW_COORD coord, WINDOW_POINT p
) {
	t.DrawPoint(p);
	t.DrawHLine(coord, coord, coord);
};
/// --------

/// Software rendering with pixel access
/// ------------------------------------
/// Separate rendering mode that provides read and write access to backbuffer
/// pixels before it's presented.

// Enters the software-rendered pixel access mode if necessary, and returns
// `true` if successful. Does nothing if the renderer is already in this mode.
// If a mode change occurred, all surfaces are invalidated.
bool GrpBackend_PixelAccessStart(void);

// Leaves the software-rendered pixel access mode and returns to regular
// hardware-accelerated rendering if necessary, and returns `true` if
// successful. Does nothing if the renderer is already in hardware mode.
// If a mode change occurred, all surfaces are invalidated.
bool GrpBackend_PixelAccessEnd(void);

// Locks the backbuffer, returning a pointer to its pixels and the row pitch.
// Should return a pitch of 0 on failure.
// On success, the returned buffer has a size of [GRP_RES.h] times the returned
// pitch, and uses the pixel format returned by GrpBackend_PixelFormat().
std::tuple<std::byte *, size_t> GrpBackend_PixelAccessLock(void);

// Unlocks the backbuffer.
void GrpBackend_PixelAccessUnlock(void);

// Calls [func] with a locked backbuffer.
void GrpBackend_PixelAccessEdit(auto func)
{
	const auto [pixels, pitch] = GrpBackend_PixelAccessLock();
	if(pitch == 0) {
		return;
	}
	std::visit(
		[&](auto P) { func.template operator()<decltype(P)>(pixels, pitch); },
		GrpBackend_PixelFormat()
	);
	GrpBackend_PixelAccessUnlock();
}
/// ------------------------------------

#ifdef WIN32_VINTAGE
	#include "platform/windows_vintage/DD_UTY.H"
#else
	#include "platform/sdl/graphics_sdl.h"
#endif
