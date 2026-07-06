/*
 *   BombEfc.h   : 爆発系エフェクト
 *
 */

#ifndef PBG_BOMBEFC_VERSION
#define PBG_BOMBEFC_VERSION	"爆発系エフェクト : Version 0.01 : Update 2000/11/21"

#include "hatoyama/logic/ffi.h"



/***** [ 定数 ] *****/
#define EXBOMB_MAX			3			// エフェクトの同時発生数
#define EXBOMB_STD			0			// よくあるタイプの爆発(??)
#define EXBOMB_OBJMAX		200			// エフェクト補助用オブジェクトの個数



/***** [構造体] *****/
typedef struct tagSpObj {
	int			x,y;
	int			vx,vy;
	uint8_t	d;
} SpObj;

typedef struct tagBombEfcCtrl {
	int			x,y;					// エフェクトの中心座標
	bool8_t	bIsUsed;	// この構造体は使用中か
	uint32_t	count;	// フレームカウンタ

	SpObj		Obj[EXBOMB_OBJMAX];		// エフェクト補助用オブジェクト

	uint8_t	type;	// エフェクトの種類
} BombEfcCtrl;


#ifdef __cplusplus
using BOMBEFC_DATA_CSPAN = std::span<const BombEfcCtrl, EXBOMB_MAX>;
#endif


#endif
