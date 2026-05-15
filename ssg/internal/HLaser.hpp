/*                                                                           */
/*   HomingL.h   長いレーザーの処理                                          */
/*                                                                           */
/*                                                                           */


#ifndef PBGWIN_HOMINGL_H
#define PBGWIN_HOMINGL_H		"HOMINGL : Ver 0.01 : Updata 2000/09/04"
//#pragma message(PBGWIN_HOMINGL_H)

#include "ssg/internal/ExDef.hpp"
#include "hatoyama/logic/coords.h"

struct HOOKS;
class C_VIV;



///// [ 定数 ] /////
#define HLASER_MAX			162
#define HLASER_LEN			7		// 描画枚数..
#define HLASER_SECTION		4		// 読み込み幅
constexpr auto HLASER_SEGMENTS = (HLASER_LEN * HLASER_SECTION);

constexpr WORLD_COORD HOMINGL_WIDTH = PixelToWorld(8);


#define HL_NONE		0			// ただ進むだけ
#define HL_TYPE1	1			// その１

#define HLS_NORM	0x00		// ホーミングレーザー通常
#define HLS_CLEAR	0x01		// ホーミングレーザー消去中
#define HLS_DEAD	0xff		// ホーミングレーザー削除要請



///// [マクロ] /////
constexpr int HLASER_GETNEXT(int current)
{
	// 後で mod -> and に変更すること //
	return ((current + (HLASER_SEGMENTS - 1)) % HLASER_SEGMENTS);
}

constexpr int HLASER_GETPREV(int current, int n)
{
	// 後で mod -> and に変更すること //
	return ((current + n) % HLASER_SEGMENTS);
}



///// [構造体] /////

// ホーミングレーザー //
struct HLaserData {
	int		Current;	// 現在の先頭
	int		v;			// 速度
	int		a;			// 加速度

	uint32_t	Count;	// フレームカウンタ

	uint8_t	Type;	// 種類(加速＆ホーミングタイプ)
	uint8_t	State;	// 状態
	uint8_t	c;	// 色
	uint8_t	Left;	// 残りホーミング回数

	HLaserData	*Next;	// 次のレーザーへのポインタ
	DegPoint	p[HLASER_LEN*HLASER_SECTION];	// 頂点キュー(ExDef.h)
};

// ホーミングレーザーセット情報 //
struct HLaserInfo {
	int		x,y;		// 中心座標

	uint8_t	d;	// 角度
	uint8_t	dw;	// 角度の開き
	uint8_t	n;	// 本数

	uint8_t	c;	// 色
	uint8_t	type;	// 種類
};



class C_HLASER {
private:
	HOOKS& Hooks;
	C_VIV& Viv;

	// ホーミングレーザーの本数
	uint16_t HLaserNow;

	// ホーミングレーザー格納バッファ
	HLaserData HLaserBuf[HLASER_MAX];

	// 確保済みホーミングレーザー
	HLaserData ActiveHL;

	// 解放済みホーミングレーザー
	HLaserData FreeHL;

public:
	C_HLASER(HOOKS& Hooks, C_VIV& Viv) noexcept : Hooks(Hooks), Viv(Viv) {
	}

	// ホーミングレーザーの初期化を行う
	void Init(void);

	// ホーミングレーザーをセットする
	void Set(const HLaserInfo *hinfo);

	// ホーミングレーザーを動作させる
	void Move(void);

	// ホーミングレーザーに消去エフェクトをセット
	void Clear(void);

	const HLaserData& Data(void) const {
		return ActiveHL;
	}

	uint16_t NumAlive(void) const {
		return HLaserNow;
	}
};



#endif
