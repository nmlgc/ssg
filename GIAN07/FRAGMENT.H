/*************************************************************************************************/
/*   FRAGMENT.H   破片処理関数群                                                                 */
/*                                                                                               */
/*************************************************************************************************/

#ifndef PBGWIN_FRAGMENT_H
#define PBGWIN_FRAGMENT_H		"FRAGMENT : Ver 0.10 : Update 99/10/31"
//#pragma message(PBGWIN_FRAGMENT_H)

import std.compat;

class C_RNG;

//// 破片定数 ////
#define FRAGMENT_MAX	1000		// 破片の最大数
#define FRG_EVADE		0x00		// かすり(開発中)
#define FRG_SMOKE		0x01		// 煙その１
#define FRG_FATCIRCLE	0x02		// 赤丸...
#define FRG_STAR1		0x03		// お星様１
#define FRG_STAR2		0x04		// お星様２
#define FRG_HIT			0x05		// ショットがヒットした
#define FRG_STAR3		0x06
#define FRG_HEART		0x07		// ハート型

#define FRG_ESCAPE		0x10		// 指定座標から逃げる
#define FRG_APPROACH	0x20		// 指定座標に近づく


//// 破片構造体 ////
struct FRAGMENT_DATA {
	int		x,y;		// 現在の座標
	int		vx,vy;		// 速度成分 (x64)
	uint8_t	count;	// フレームカウンタ(０の時は使用していないとする)
	uint8_t	cmd;	// どんな破片？
};

using FRAGMENT_DATA_CSPAN = std::span<const FRAGMENT_DATA, FRAGMENT_MAX>;


//// 破片用変数 ////
class C_FRAGMENT {
private:
	C_RNG& RNG;

	// 次に破片データを挿入する位置
	int FragmentPtr = 0;

	FRAGMENT_DATA Fragment[FRAGMENT_MAX];

public:
	C_FRAGMENT(C_RNG& RNG) noexcept : RNG(RNG) {
	}

	//// 破片関数 ////
	void Set(int x, int y, uint8_t cmd);
	void Move(void);
	void Setup(void);

	FRAGMENT_DATA_CSPAN Data(void) const {
		return Fragment;
	}
};

#endif
