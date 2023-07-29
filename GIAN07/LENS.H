/*                                                                           */
/*   Lens.h   レンズエフェクト                                               */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_LENS_H
#define PBGWIN_LENS_H		"LENS_EFC : Version 0.01 : Update 2000/09/18"
#pragma message(PBGWIN_LENS_H)

#include "game/coords.h"
#include <memory>
#include <optional>
#include <stdint.h>

struct SURFACE_DDRAW; // TODO: Remove once we use IDs


///// [構造体] /////

// レンズデータ定義用構造体 //
struct LensInfo {
	uint16_t	r;	// レンズの半径
	uint16_t	Height;	// レンズの直径
	std::unique_ptr<uint32_t[]>	Data;	// レンズ置換テーブル

	SURFACE_DDRAW*	gTemp;	// データ一時待避用Surface(できればSysMemory)
	size_t	gPitch;		// Surface の横幅
	//BYTE		gID;		// SurfaceID (DD_UTY の関数に対する)

	// GrpLock() 系関数 : レンズボールを描画する //
	void Draw(WINDOW_POINT center);
};

extern std::optional<LensInfo> Lens;



///// [ 関数 ] /////

// 半径:r  出っ張り:m  のレンズを作成 //
std::optional<LensInfo> GrpCreateLensBall(
	uint16_t r, uint16_t m, SURFACE_DDRAW& surf
);



#endif
