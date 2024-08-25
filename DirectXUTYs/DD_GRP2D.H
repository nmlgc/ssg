/*                                                                           */
/*   DD_GRP2D.h   ２Ｄ＆８Ｂｉｔ時の図形描画関数                             */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_DD_GRP2D_H
#define PBGWIN_DD_GRP2D_H		"DD_GRP2D : Version 0.03 : Update 1999/01/12"


// 更新履歴 //

// 2000/ 1/12 : α関連の関数を一部追加
// 1999/12/03 : ２Ｄ図形描画作成(αをのぞく)


// ヘッダファイル //
#include "platform/graphics_backend.h"

class GRAPHICS_GEOMETRY_DDRAW : public GRAPHICS_GEOMETRY {
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

	void Setup(void);	// 描画系関数ポインタをセットする
};

void GrpPoint(WINDOW_POINT p);
void GrpHLine(int x1, int x2, int y);
void DisableAlphaTable(void);



// インライン関数 //
inline bool _2DChkPoint(int x, int y)
{
	extern int CLIP_X_MIN,CLIP_X_MAX,CLIP_Y_MIN,CLIP_Y_MAX;

	return !(
		(x < CLIP_X_MIN) ||
		(x > CLIP_X_MAX) ||
		(y < CLIP_Y_MIN) ||
		(y > CLIP_Y_MAX)
	);
}

// 正方形クリップ : ReturnValue true:クリッピング成功 false:完全に範囲外
inline bool _2DClipRect(int *x1, int *y1, int *x2, int *y2)
{
	extern int CLIP_X_MIN,CLIP_X_MAX,CLIP_Y_MIN,CLIP_Y_MAX;

	// 完全判定 //
	if((*x1 > CLIP_X_MAX) || (*x2 < CLIP_X_MIN)) {
		return false;
	}
	if((*y1 > CLIP_Y_MAX) || (*y2 < CLIP_Y_MIN)) {
		return false;
	}

	// クリッピング //
	if((*x1)<CLIP_X_MIN) (*x1)=CLIP_X_MIN;
	if((*y1)<CLIP_Y_MIN) (*y1)=CLIP_Y_MIN;
	if((*x2)>CLIP_X_MAX) (*x2)=CLIP_X_MAX;
	if((*y2)>CLIP_Y_MAX) (*y2)=CLIP_Y_MAX;

	return true;
}

#endif
