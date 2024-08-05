/*                                                                           */
/*   DD_GRP3D.cpp   ３Ｄ時の図形描画関数                                     */
/*                                                                           */
/*                                                                           */

#include "DD_UTY.H"
#include "constants.h"
#include "DD_GRP3D.H"
#include "game/ut_math.h"
#include <d3d.h>
#pragma message(PBGWIN_DD_GRP3D_H)


// インライン関数 //
inline void Set2DPoint(D3DTLVERTEX *tlv, int x, int y)
{
	extern BYTE	 Col3D_R,Col3D_G,Col3D_B,Col3D_A;

	memset(tlv,0,sizeof(D3DTLVERTEX));

	tlv->sx       = D3DVAL(x);
	tlv->sy       = D3DVAL(y);
	tlv->sz       = D3DVAL(0);
	tlv->rhw      = D3DVAL(1);
	tlv->color    = RGBA_MAKE(Col3D_R,Col3D_G,Col3D_B,Col3D_A);
	tlv->specular = RGB_MAKE(0,0,0);
}

inline void Set2DPointC(D3DTLVERTEX *tlv, int x, int y, D3DCOLOR c)
{
	memset(tlv,0,sizeof(D3DTLVERTEX));

	tlv->sx       = D3DVAL(x);
	tlv->sy       = D3DVAL(y);
	tlv->sz       = D3DVAL(0);
	tlv->rhw      = D3DVAL(1);
	tlv->color    = c;
	tlv->specular = RGB_MAKE(0,0,0);
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


// ３Ｄ用変数 //
BOOL			bLocked = FALSE;					// ロック中か
BYTE			Col3D_R,Col3D_G,Col3D_B;			// 現在の色
BYTE			Col3D_A;							// α
D3DTLVERTEX		work[100];							// 頂点用ワークエリア


// 描画系関数ポインタをセットする //
void _3DSetup(void)
{
	GrpLock     = _3DLock;
	GrpUnlock   = _3DUnlock;
	GrpSetColor = _3DSetColor;
	GrpSetAlpha = _3DSetAlpha;
	GrpLine     = _3DLine;
	GrpBox      = _3DBox;
	GrpCircle   = _3DCircle;
	GrpCircleF  = _3DCircleF;
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
	Col3D_R = rgb.r;
	Col3D_G = rgb.g;
	Col3D_B = rgb.b;
}

// αセット //
void _3DSetAlpha(uint8_t a, uint8_t mode)
{
	Col3D_A = a;

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

// 円 //
void _3DCircle(int x, int y, int r)
{
	if(!bLocked) return;

	for(auto i = 0; i <= 32; i++) { // 円周上の点
		Set2DPoint(work+i,x+cosl(i*8,r),y+sinl(i*8,r));
	}

	DxObj.D3Dev->DrawPrimitive(
		D3DPT_LINESTRIP, D3DVT_TLVERTEX, work, 33, D3DDP_WAIT
	);
}

// 円塗りつぶし //
void _3DCircleF(int x, int y, int r)
{
	int		i;

	if(!bLocked) return;

	Set2DPoint(work,x,y);		// 円の中心
	for(i=0;i<=32;i++){			// 円周上の点
		Set2DPoint(work+i+1,x+cosl(i*8,r),y+sinl(i*8,r));
	}
	DxObj.D3Dev->DrawPrimitive(D3DPT_TRIANGLEFAN,D3DVT_TLVERTEX,work,34,D3DDP_WAIT);
}

// ポリゴン塗りつぶし //
void _3DPolygon(std::span<const WINDOW_POINT> p)
{
	if(!bLocked || (p.size() < 3)) {
		return;
	}

	for(size_t i = 0; i < p.size(); i++) {
		Set2DPoint(work+i,p[i].x,p[i].y);
	}

	DxObj.D3Dev->DrawPrimitive(
		D3DPT_TRIANGLEFAN, D3DVT_TLVERTEX, work, p.size(), D3DDP_WAIT
	);
}

// α長方形 //
void _3DBoxA(int x1, int y1, int x2, int y2)
{
	if(!bLocked) return;
	AlphaEnable(TRUE);
	_3DBox(x1, y1, x2, y2);
	AlphaEnable(FALSE);
}

// α円塗りつぶし //
void GrpCircleFA(int x, int y, int r)
{
	int		i;

	if(!bLocked) return;
	AlphaEnable(TRUE);

	Set2DPoint(work,x,y);		// 円の中心
	for(i=0;i<=32;i++){			// 円周上の点
		Set2DPoint(work+i+1,x+cosl(i*8,r),y+sinl(i*8,r));
	}
	DxObj.D3Dev->DrawPrimitive(D3DPT_TRIANGLEFAN,D3DVT_TLVERTEX,work,34,D3DDP_WAIT);

	AlphaEnable(FALSE);
}

// αポリゴン塗りつぶし //
void GrpPolygonA(std::span<const WINDOW_POINT> p)
{
	if(!bLocked || (p.size() < 3)) {
		return;
	}

	AlphaEnable(TRUE);

	for(size_t i = 0; i < p.size(); i++) {
		Set2DPoint(work+i,p[i].x,p[i].y);
	}

	DxObj.D3Dev->DrawPrimitive(
		D3DPT_TRIANGLEFAN, D3DVT_TLVERTEX, work, p.size(), D3DDP_WAIT
	);

	AlphaEnable(FALSE);
}

// 太い半透明円描画 //
void GrpFatCircleA(int x, int y, int r, int w)
{
	int		i;
	int		lx,ly;
	int		wx,wy;

	// 普通の円になってしまう場合 //
	if(w >= r) {
		GrpCircleFA(x, y, (r + w));
	}

	if(!bLocked) return;
	AlphaEnable(TRUE);

	for(i=0;i<=32;i++){
		lx = cosl(i*8,r);	ly = sinl(i*8,r);
		wx = cosl(i*8,w);	wy = sinl(i*8,w);
		Set2DPoint(work+i*2  ,x+lx-wx,y+ly-wy);
		Set2DPoint(work+i*2+1,x+lx+wx,y+ly+wy);
	}
	DxObj.D3Dev->DrawPrimitive(D3DPT_TRIANGLESTRIP,D3DVT_TLVERTEX,work,33*2,D3DDP_WAIT);

	AlphaEnable(FALSE);
}

// グラデーション付き長方形(中心部が明るくなる) //
void GrpGrdRect(std::span<const WINDOW_POINT, 4> p)
{
	const DWORD c1 = RGBA_MAKE(Col3D_R, Col3D_G, Col3D_B, Col3D_A);
	const DWORD c2 = RGBA_MAKE(255, 255, 255, Col3D_A);
	const DWORD c3 = c1; //RGBA_MAKE(Col3D_R>>1,Col3D_G>>1,Col3D_B>>1,Col3D_A);

	if(!bLocked) return;

	Set2DPointC(work+0,p[3].x,p[3].y,c3);
	Set2DPointC(work+1,p[2].x,p[2].y,c1);

	Set2DPointC(work+2,(p[0].x+p[3].x)>>1,(p[0].y+p[3].y)>>1,c2);
	Set2DPointC(work+3,(p[1].x+p[2].x)>>1,(p[1].y+p[2].y)>>1,c2);

	Set2DPointC(work+4,p[0].x,p[0].y,c3);
	Set2DPointC(work+5,p[1].x,p[1].y,c1);

	DxObj.D3Dev->DrawPrimitive(D3DPT_TRIANGLESTRIP,D3DVT_TLVERTEX,work,6,D3DDP_WAIT);
}

// グラデーション付き長方形(中心部が明るくなる) //
void GrpGrdRectA(std::span<const WINDOW_POINT, 4> p)
{
	if(!bLocked) return;
	AlphaEnable(TRUE);
	GrpGrdRect(p);
	AlphaEnable(FALSE);
}

// グラデーション付きポリゴン(レーザー用？) //
void GrpGrdPolygonA(std::span<const WINDOW_POINT> p)
{
	// この関数では最初に指定された頂点の色だけを白とし、他の頂点色は  //
	// GrpSetColor()で指定されている色なる。なお、αの指定も有効となる //
	const DWORD c1 = RGBA_MAKE(Col3D_R, Col3D_G, Col3D_B, Col3D_A);
	constexpr DWORD c2 = RGB_MAKE(255, 255, 255);

	if(!bLocked) return;
	AlphaEnable(TRUE);

	Set2DPointC(work+0,p[0].x,p[0].y,c2);
	for(size_t i = 1; i < p.size(); i++) {
		Set2DPointC(&work[i], p[i].x, p[i].y, c1);
	}

	DxObj.D3Dev->DrawPrimitive(
		D3DPT_TRIANGLEFAN, D3DVT_TLVERTEX, work, p.size(), D3DDP_WAIT
	);

	AlphaEnable(FALSE);
}

void GrpGrdLineEx(int x, int y1, int y2)
{
	DWORD	c1,c2;

	if(!bLocked) return;

	//GrpSetAlpha(128,ALPHA_NORM);
	c1 = RGBA_MAKE(200,0,0,Col3D_A);
	c2 = RGBA_MAKE(250,250,0,Col3D_A);

	//AlphaEnable(TRUE);
	Set2DPointC(work+0,x,y1,c1);
	Set2DPointC(work+1,x,y2,c2);

	DxObj.D3Dev->DrawPrimitive(D3DPT_LINELIST,D3DVT_TLVERTEX,work,2,D3DDP_WAIT);

	//AlphaEnable(FALSE);
}
