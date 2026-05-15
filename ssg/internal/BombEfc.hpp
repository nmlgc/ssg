/*
 *   BombEfc.h   : 爆発系エフェクト
 *
 */

#ifndef PBG_BOMBEFC_VERSION
#define PBG_BOMBEFC_VERSION	"爆発系エフェクト : Version 0.01 : Update 2000/11/21"

import std.compat;

class C_RNG;



/***** [ 定数 ] *****/
#define EXBOMB_MAX			3			// エフェクトの同時発生数
#define EXBOMB_STD			0			// よくあるタイプの爆発(??)
#define EXBOMB_OBJMAX		200			// エフェクト補助用オブジェクトの個数



/***** [構造体] *****/
struct SpObj {
	int			x,y;
	int			vx,vy;
	uint8_t	d;
};

struct BombEfcCtrl {
	int			x,y;					// エフェクトの中心座標
	bool	bIsUsed;	// この構造体は使用中か
	uint32_t	count;	// フレームカウンタ

	SpObj		Obj[EXBOMB_OBJMAX];		// エフェクト補助用オブジェクト

	uint8_t	type;	// エフェクトの種類
};

using BOMBEFC_DATA_CSPAN = std::span<const BombEfcCtrl, EXBOMB_MAX>;

class C_BOMBEFC {
private:
	C_RNG& RNG;

	BombEfcCtrl BombEfc[EXBOMB_MAX];

public:
	C_BOMBEFC(C_RNG& RNG) noexcept : RNG(RNG) {
	}

	// 爆発系エフェクトの初期化
	void Init(void);

	// 爆発系エフェクトをセットする
	void Set(int x, int y, uint8_t type);

	// 爆発系エフェクトを動作させる
	void Move(void);

	BOMBEFC_DATA_CSPAN Data(void) const {
		return BombEfc;
	}
};




#endif
