/*                                                                           */
/*   Lens.h   レンズエフェクト                                               */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_LENS_H
#define PBGWIN_LENS_H		"LENS_EFC : Version 0.01 : Update 2000/09/18"
#pragma message(PBGWIN_LENS_H)

#include "DirectXUTYs/PBGUTY_X.H"



///// [構造体] /////

// レンズデータ定義用構造体 //
typedef struct tagLensInfo {
	uint16_t	r;	// レンズの半径
	uint16_t	Height;	// レンズの直径
	DWORD		*Data;		// レンズ置換テーブル

	GRP			gTemp;		// データ一時待避用Surface(できればSysMemory)
	DWORD		gPitch;		// Surface の横幅
	//BYTE		gID;		// SurfaceID (DD_UTY の関数に対する)
} LensInfo;

extern LensInfo *Lens;



///// [ 関数 ] /////

// 半径:r  出っ張り:m  のレンズを作成 //
LensInfo* GrpCreateLensBall(uint16_t r, uint16_t m, GRP id);

// GrpLock() 系関数 : レンズボールを描画する //
extern void GrpDrawLensBall(LensInfo *LInfo, int x, int y);

// レンズボール用データを破棄する //
extern void GrpReleaseLensBall(LensInfo *LInfo);



#endif
