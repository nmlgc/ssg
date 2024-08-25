/*                                                                           */
/*   DD_GRP3D.h   ３Ｄ時の図形描画関数                                       */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_DD_GRP3D_H
#define PBGWIN_DD_GRP3D_H		"DD_GRP3D : Version 0.03 : Update 2000/02/22"


// 更新履歴 //

// 2000/02/22 : GrpGrdPolygon() を追加
// 1999/12/04 : レーザー用特殊α関数を追加
// 1999/12/02 : やっと一部の関数が動作し始めた。16Bit-αはやっぱり綺麗だな...


// ヘッダファイル //
#include "platform/graphics_backend.h"

class GRAPHICS_GEOMETRY_D3D : public GRAPHICS_GEOMETRY {
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
};

// ３Ｄ専用関数 //
void GrpLineStrip(VERTEX_XY_SPAN<> xys);
void GrpTriangles(
	TRIANGLE_PRIMITIVE, VERTEX_XY_SPAN<>, VERTEX_RGBA_SPAN<> colors = {}
);
void GrpTrianglesA(
	TRIANGLE_PRIMITIVE, VERTEX_XY_SPAN<>, VERTEX_RGBA_SPAN<> colors = {}
);
void GrpGrdLineEx(int x, int y1, int y2);	// スペアな用グラデーションライン


#endif
