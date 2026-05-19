/*                                                                           */
/*   LLaser.h   長いレーザーの処理                                           */
/*                                                                           */
/*                                                                           */


#ifndef PBGWIN_LLASER_H
#define PBGWIN_LLASER_H		"LLASER : Ver 0.03 : Updata 2000/02/07"
//#pragma message(PBGWIN_LLASER_H)

// 更新履歴 //
// 2000/05/29 : ８ビットモード描画関連のＢｕｇＦｉｘ
// 2000/03/22 : レーザー関数のＩＤの意味がレーザー配列のＩＤからその敵が
//            : 発射しているレーザーの何番目か、に変更された


import std.compat;
#include "hatoyama/api/export.h"
#include "hatoyama/logic/coords.h"


class C_VIV;
struct ENEMY_DATA;
struct HOOKS;



//// レーザー用定数２ ////
#define LLASER_MAX		20
#define LLASER_EVADE	1		// レーザーのかすり値


//// レーザーの種類定数２
#define LLS_LONG		0x00
#define LLS_LONGY		0x01
#define LLS_SETDEG		0x02
#define LLS_LONGZ		0x03	// 自機セット


//// レーザーフラグ２ ////
#define LLF_DISABLE		0x00			// レーザーが使用されていない
#define LLF_NORM		0x01			// レーザーが完全に開ききった
#define LLF_OPEN		0x02			// レーザを開いている
#define LLF_CLOSE		0x04			// レーザーを閉じている
#define LLF_CLOSEL		0x08			// レーザーをライン状態にする
#define LLF_LINE		0x10			// レーザーはライン状態



//// レーザー発動コマンド構造体２ ////
struct LLASER_CMD {
	ENEMY_DATA	*e;		// 敵データへのポインタ

	int		dx,dy;		// レーザーの発射座標ずらし値
	int		v;			// レーザーの速度

	int		w;			// レーザーの太さ最終値

	uint8_t	d;	// レーザー発射角

	uint8_t	c;	// レーザーの色
	uint8_t	type;	// レーザーの種類
};


//// レーザー用構造体２ ////
struct LLASER_DATA {
	// 敵データへのポインタ(ここら辺でボスでも雑魚でも発射できるように)
	ENEMY_DATA *e;

	WORLD_COORD x, y;	// 現在の表示座標
	WORLD_COORD dx, dy;	// 敵データからのずらし値(x64)
	int lx, ly;	// レーザー円の中心座標までのベクトル(Grp)
	int infx, infy;	// 仮の無限遠へのベクトル(Grp)
	int wx, wy;	// レーザー幅(Grp)

	WORLD_COORD w, wmax; // 幅、最大幅(x64)
	WORLD_COORD v;

	uint32_t	count;	// フレームカウンタ

	WINDOW_POINT	p[4];	// 座標保持用のポインタ(Grp)

	uint8_t	d;	// レーザーの発射角
	uint8_t	c;	// レーザーの色

	uint8_t	flag;	// レーザーの状態
	uint8_t	type;	// レーザーの種類
	uint8_t	EnemyID;	// 敵から見た番号
};

using LLASER_DATA_CSPAN = std::span<const LLASER_DATA, LLASER_MAX>;



class C_LLASER {
public:
	LLASER_CMD LLaserCmd;

private:
	HOOKS& Hooks;
	C_VIV& Viv;

	LLASER_DATA LLaser[LLASER_MAX];

	void _LLaserHitCheck(const LLASER_DATA *lp);

	// レーザーの座標をセットする
	void _LLaserXYSet(int id);

public:
	C_LLASER(HOOKS& Hooks, C_VIV& Viv) noexcept : Hooks(Hooks), Viv(Viv) {
	}

	// レーザーをセットする
	bool Set(uint8_t id);

	// レーザーを開く
	void Open(const ENEMY_DATA *e, uint8_t id);

	// レーザーを閉じる
	void Close(const ENEMY_DATA *e, uint8_t id);

	// レーザーをライン状態にする
	void Line(const ENEMY_DATA *e, uint8_t id);

	// レーザーを角度絶対で回転
	void DegA(const ENEMY_DATA *e, uint8_t d, uint8_t id);

	// レーザーを角度相対で回転);
	void DegR(const ENEMY_DATA *e, char d, uint8_t id);

	// 敵に関連づけられたレーザーを強制クローズ
	void ForceClose(const ENEMY_DATA *e);

	// レーザーを動かす
	void Move(void);

	// 無限遠レーザーを全クローズ
	void Clear(void);

	// レーザー配列の初期化をする
	HATOYAMA_API void Setup(void);

	const LLASER_DATA_CSPAN Data(void) const noexcept {
		return LLaser;
	}
};



#endif
