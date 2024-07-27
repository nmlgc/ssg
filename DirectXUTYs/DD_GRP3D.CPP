/*                                                                           */
/*   DD_GRP3D.cpp   ３Ｄ時の図形描画関数                                     */
/*                                                                           */
/*                                                                           */

#include "DD_UTY.H"
#include "constants.h"
#include "DD_GRP3D.H"
#include "game/enum_array.h"
#include "game/ut_math.h"
#include <assert.h>
#include <d3d.h>
#pragma message(PBGWIN_DD_GRP3D_H)


// ３Ｄ用変数 //
bool	bLocked = false;	// ロック中か
RGBA	Col3D;	// 現在の色+α
D3DTLVERTEX	work[100];	// 頂点用ワークエリア


// インライン関数 //
inline void Set2DPoint(D3DTLVERTEX *tlv, int x, int y, RGBA c = Col3D)
{
	memset(tlv,0,sizeof(D3DTLVERTEX));

	tlv->sx       = D3DVAL(x);
	tlv->sy       = D3DVAL(y);
	tlv->sz       = D3DVAL(0);
	tlv->rhw      = D3DVAL(1);
	tlv->color    = RGBA_MAKE(c.r, c.g, c.b, c.a);
	tlv->specular = RGB_MAKE(0,0,0);
}

void DrawVertices(
	D3DPRIMITIVETYPE type, VERTEX_XY_SPAN<> xys, VERTEX_RGBA_SPAN<> colors
)
{
	const auto count = xys.size();
	assert(count <= (sizeof(work) / sizeof(work[0])));
	auto out_p = work;
	if(colors.empty()) {
		for(const auto point : xys) {
			Set2DPoint(out_p++, point.x, point.y);
		}
	} else {
		for(const auto [point, color] : std::views::zip(xys, colors)) {
			Set2DPoint(out_p++, point.x, point.y, color);
		}
	}
	DxObj.D3Dev->DrawPrimitive(type, D3DVT_TLVERTEX, work, count, D3DDP_WAIT);
}

inline bool AlphaEnable(bool flag)
{
	HRESULT dxret;

	dxret = DxObj.D3Dev->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE,flag);
	if(dxret!=D3D_OK){
		if(!DxObj.Restore()) {
			return false;
		}
	}

	return TRUE;
}


// 画面描画の準備をする //
void GRAPHICS_GEOMETRY_D3D::Lock(void)
{
	D3DRECT d3drc = { 0, 0, GRP_RES.w, GRP_RES.h };
	HRESULT			dxret;

	// 目標セット //
	dxret = DxObj.D3Dev->SetRenderTarget(DxObj.Back,0);
	if(dxret!=D3D_OK){
		if(!DxObj.Restore()) {
			return;
		}
	}

	// Ｚバッファだけをクリアする //
	dxret = DxObj.View->Clear(1,&d3drc,/*D3DCLEAR_TARGET|*/D3DCLEAR_ZBUFFER);
	if(dxret!=D3D_OK){
		if(!DxObj.Restore()) {
			return;
		}
	}

	// 描き込み開始 //
	dxret = DxObj.D3Dev->BeginScene();
	if(dxret!=D3D_OK) return;

	bLocked = TRUE;
	return;
}

// 画面描画を完了する //
void GRAPHICS_GEOMETRY_D3D::Unlock(void)
{
	// 描き込み終了 //
	DxObj.D3Dev->EndScene();
	bLocked = FALSE;
}

// 色セット //
void GRAPHICS_GEOMETRY_D3D::SetColor(RGB216 col)
{
	const auto rgb = col.ToRGB();
	Col3D.r = rgb.r;
	Col3D.g = rgb.g;
	Col3D.b = rgb.b;
}

void GRAPHICS_GEOMETRY_D3D::SetAlphaNorm(uint8_t a)
{
	Col3D.a = a;
	DxObj.D3Dev->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_BOTHSRCALPHA);
	DxObj.D3Dev->SetRenderState(
		D3DRENDERSTATE_DESTBLEND, D3DBLEND_BOTHSRCALPHA
	);
}

void GRAPHICS_GEOMETRY_D3D::SetAlphaOne(void)
{
	Col3D.a = 0xFF;
	DxObj.D3Dev->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
	DxObj.D3Dev->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
}

// 直線 //
void GRAPHICS_GEOMETRY_D3D::DrawLine(int x1, int y1, int x2, int y2)
{
	if(!bLocked) return;

	Set2DPoint(work+0,x1,y1);
	Set2DPoint(work+1,x2,y2);

	DxObj.D3Dev->DrawPrimitive(D3DPT_LINELIST,D3DVT_TLVERTEX,work,2,D3DDP_WAIT);
}

// 長方形 //
void GRAPHICS_GEOMETRY_D3D::DrawBox(int x1, int y1, int x2, int y2)
{
	if(!bLocked) return;

	Set2DPoint(work+0,x1,y1);
	Set2DPoint(work+1,x2,y1);
	Set2DPoint(work+2,x1,y2);
	Set2DPoint(work+3,x2,y2);

	DxObj.D3Dev->DrawPrimitive(D3DPT_TRIANGLESTRIP,D3DVT_TLVERTEX,work,4,D3DDP_WAIT);
}

void GRAPHICS_GEOMETRY_D3D::DrawLineStrip(VERTEX_XY_SPAN<> xys)
{
	if(!bLocked) return;
	DrawVertices(D3DPT_LINESTRIP, xys, {});
}

constexpr ENUMARRAY<D3DPRIMITIVETYPE, TRIANGLE_PRIMITIVE> PRIMITIVE_TYPES = {
	D3DPT_TRIANGLEFAN, D3DPT_TRIANGLESTRIP
};

void GRAPHICS_GEOMETRY_D3D::DrawTriangles(
	TRIANGLE_PRIMITIVE tp, VERTEX_XY_SPAN<> xys, VERTEX_RGBA_SPAN<> colors
)
{
	if(!bLocked || (xys.size() < 3)) {
		return;
	}
	DrawVertices(PRIMITIVE_TYPES[tp], xys, colors);
}

void GRAPHICS_GEOMETRY_D3D::DrawTrianglesA(
	TRIANGLE_PRIMITIVE tp, VERTEX_XY_SPAN<> xys, VERTEX_RGBA_SPAN<> colors
)
{
	if(!bLocked || (xys.size() < 3)) {
		return;
	}
	AlphaEnable(TRUE);
	DrawVertices(PRIMITIVE_TYPES[tp], xys, colors);
	AlphaEnable(FALSE);
}

void GRAPHICS_GEOMETRY_D3D::DrawTriangleFan(VERTEX_XY_SPAN<> xys)
{
	return DrawTriangles(TRIANGLE_PRIMITIVE::FAN, xys);
}

// α長方形 //
void GRAPHICS_GEOMETRY_D3D::DrawBoxA(int x1, int y1, int x2, int y2)
{
	if(!bLocked) return;
	AlphaEnable(TRUE);
	DrawBox(x1, y1, x2, y2);
	AlphaEnable(FALSE);
}

void GRAPHICS_GEOMETRY_D3D::DrawGrdLineEx(int x, int y1, int y2)
{
	if(!bLocked) return;

	// SetAlpha(128, GRAPHICS_ALPHA::NORM);
	constexpr RGB c1 = { 200,   0, 0 };
	constexpr RGB c2 = { 250, 250, 0 };

	//AlphaEnable(TRUE);
	Set2DPoint(&work[0], x, y1, c1.WithAlpha(Col3D.a));
	Set2DPoint(&work[1], x, y2, c2.WithAlpha(Col3D.a));

	DxObj.D3Dev->DrawPrimitive(D3DPT_LINELIST,D3DVT_TLVERTEX,work,2,D3DDP_WAIT);

	//AlphaEnable(FALSE);
}
