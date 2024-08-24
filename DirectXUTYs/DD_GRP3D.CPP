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


// 描画系関数ポインタをセットする //
void _3DSetup(void)
{
	GrpLock     = _3DLock;
	GrpUnlock   = _3DUnlock;
	GrpSetColor = _3DSetColor;
	GrpSetAlpha = _3DSetAlpha;
	GrpLine     = _3DLine;
	GrpBox      = _3DBox;
	GrpPolygon  = _3DPolygon;
	GrpBoxA     = _3DBoxA;
}

// 画面描画の準備をする //
void _3DLock(void)
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
void _3DUnlock(void)
{
	// 描き込み終了 //
	DxObj.D3Dev->EndScene();
	bLocked = FALSE;
}

// 色セット //
void _3DSetColor(RGB216 col)
{
	const auto rgb = col.ToRGB();
	Col3D.r = rgb.r;
	Col3D.g = rgb.g;
	Col3D.b = rgb.b;
}

// αセット //
void _3DSetAlpha(uint8_t a, uint8_t mode)
{
	Col3D.a = a;

	switch(mode){
		case(ALPHA_ONE):
			DxObj.D3Dev->SetRenderState(D3DRENDERSTATE_SRCBLEND,D3DBLEND_ONE);
			DxObj.D3Dev->SetRenderState(D3DRENDERSTATE_DESTBLEND,D3DBLEND_ONE);
		break;

		case(ALPHA_NORM):
			DxObj.D3Dev->SetRenderState(D3DRENDERSTATE_SRCBLEND,D3DBLEND_BOTHSRCALPHA);
			DxObj.D3Dev->SetRenderState(D3DRENDERSTATE_DESTBLEND,D3DBLEND_BOTHSRCALPHA);
		break;
	}
}

// 直線 //
void _3DLine(int x1, int y1, int x2, int y2)
{
	if(!bLocked) return;

	Set2DPoint(work+0,x1,y1);
	Set2DPoint(work+1,x2,y2);

	DxObj.D3Dev->DrawPrimitive(D3DPT_LINELIST,D3DVT_TLVERTEX,work,2,D3DDP_WAIT);
}

// 長方形 //
void _3DBox(int x1, int y1, int x2, int y2)
{
	if(!bLocked) return;

	Set2DPoint(work+0,x1,y1);
	Set2DPoint(work+1,x2,y1);
	Set2DPoint(work+2,x1,y2);
	Set2DPoint(work+3,x2,y2);

	DxObj.D3Dev->DrawPrimitive(D3DPT_TRIANGLESTRIP,D3DVT_TLVERTEX,work,4,D3DDP_WAIT);
}

void GrpLineStrip(VERTEX_XY_SPAN<> xys)
{
	if(!bLocked) return;
	DrawVertices(D3DPT_LINESTRIP, xys, {});
}

constexpr ENUMARRAY<D3DPRIMITIVETYPE, TRIANGLE_PRIMITIVE> D3DPTS = {
	D3DPT_TRIANGLEFAN, D3DPT_TRIANGLESTRIP
};

void GrpTriangles(
	TRIANGLE_PRIMITIVE tp, VERTEX_XY_SPAN<> xys, VERTEX_RGBA_SPAN<> colors
)
{
	if(!bLocked || (xys.size() < 3)) {
		return;
	}
	DrawVertices(D3DPTS[tp], xys, colors);
}

void GrpTrianglesA(
	TRIANGLE_PRIMITIVE tp, VERTEX_XY_SPAN<> xys, VERTEX_RGBA_SPAN<> colors
)
{
	if(!bLocked || (xys.size() < 3)) {
		return;
	}
	AlphaEnable(TRUE);
	DrawVertices(D3DPTS[tp], xys, colors);
	AlphaEnable(FALSE);
}

// ポリゴン塗りつぶし //
void _3DPolygon(VERTEX_XY_SPAN<> p)
{
	return GrpTriangles(TRIANGLE_PRIMITIVE::FAN, p);
}

// α長方形 //
void _3DBoxA(int x1, int y1, int x2, int y2)
{
	if(!bLocked) return;
	AlphaEnable(TRUE);
	_3DBox(x1, y1, x2, y2);
	AlphaEnable(FALSE);
}

// αポリゴン塗りつぶし //
void GrpPolygonA(VERTEX_XY_SPAN<> p)
{
	return GrpTrianglesA(TRIANGLE_PRIMITIVE::FAN, p);
}

// グラデーション付きポリゴン(レーザー用？) //
void GrpGrdPolygonA(VERTEX_XY_SPAN<> p)
{
	// この関数では最初に指定された頂点の色だけを白とし、他の頂点色は  //
	// GrpSetColor()で指定されている色なる。なお、αの指定も有効となる //
	const RGBA c1 = Col3D;
	const RGB c2 = { 255, 255, 255 };

	if(!bLocked) return;
	AlphaEnable(TRUE);

	Set2DPoint(&work[0], p[0].x, p[0].y, c2.WithAlpha(Col3D.a));
	for(size_t i = 1; i < p.size(); i++) {
		Set2DPoint(&work[i], p[i].x, p[i].y, c1);
	}

	DxObj.D3Dev->DrawPrimitive(
		D3DPT_TRIANGLEFAN, D3DVT_TLVERTEX, work, p.size(), D3DDP_WAIT
	);

	AlphaEnable(FALSE);
}

void GrpGrdLineEx(int x, int y1, int y2)
{
	if(!bLocked) return;

	//GrpSetAlpha(128,ALPHA_NORM);
	constexpr RGB c1 = { 200,   0, 0 };
	constexpr RGB c2 = { 250, 250, 0 };

	//AlphaEnable(TRUE);
	Set2DPoint(&work[0], x, y1, c1.WithAlpha(Col3D.a));
	Set2DPoint(&work[1], x, y2, c2.WithAlpha(Col3D.a));

	DxObj.D3Dev->DrawPrimitive(D3DPT_LINELIST,D3DVT_TLVERTEX,work,2,D3DDP_WAIT);

	//AlphaEnable(FALSE);
}
