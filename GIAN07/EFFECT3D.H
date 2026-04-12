/*                                                                           */
/*   Effect3D.h   ３Ｄエフェクトの処理                                       */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_EFFECT3D_H
#define PBGWIN_EFFECT3D_H	"EFFECT3D : Ver 0.01 : Update 2000/05/31"
//#pragma message(PBGWIN_EFFECT3D_H)



///// [更新履歴] /////
// 2000/05/31 : 開発開始



///// [ヘッダファイル] /////
import std.compat;
#include "hatoyama/logic/coords.h"



///// [ 定数 ] /////
#define CUBE_MAX        8
#define STAR_MAX        40
#define FAKE_ECLSTR_MAX 80
#define ROCK_MAX        28
#define S6RASTER_MAX    28
#define S6STAR_MAX      60
#define S3STAR_MAX      180

#define STG4ROCK_STDMOVE	0		// 普通のスクロールね
#define STG4ROCK_ACCMOVE1	1		// 加速有りスクロール(1)
#define STG4ROCK_ACCMOVE2	2		// 加速有りスクロール(2)
#define STG4ROCK_3DMOVE		3		// ３Ｄ回転
#define STG4ROCK_LEAVE		4		// 一時的に岩を消去する
#define STG4ROCK_END		5		// エフェクト終了



///// [ 構造体 ] /////
struct Point3D {
	WORLD_COORD x, y, z;
};

struct LineList3D {
	std::span<const WORLD_POINT>	p;	/* 頂点の座標         */

	uint8_t	DegX, DegY, DegZ;	/* 各軸に対する回転角 */
};

struct Deg3D {
	int		dx;
	int		dy;
	int		dz;
};

struct Cube3D {
	Point3D		p;
	Deg3D		d;
	int			l;
};

struct Star2D {
	int		x,y;
	int		vy;
};

// 非・汎用２Ｄ正方形ワイヤーフレーム //
struct WFLine2D {
	int		ox,oy;		// 中心座標
	int		w;			// 正方形の一辺の長さ
	uint8_t	d;	// 正方形の傾き角度
};

// 偽ＥＣＬ羅列管理用構造体 //
struct FakeECLString {
	int		SrcX, SrcY;		// 元画像の基準となるＸ＆Ｙ座標
	int		x,y;			// 現在の座標x64
	int		vx,vy;			// 現在の速度成分x64
};

// 岩管理用構造体 //
struct Rock3D {
	int		x, y, z;		// 現在の座標
	int		vx, vy;			// 速度XY成分(2D-平面上)

	uint32_t	count;	// カウンタ
	int		v;				// 速度

	char	a;				// 加速度
	uint8_t	d;	// 角度(2D-平面上)
	uint8_t	GrpID;	// グラフィックＩＤ
	uint8_t	State;	// 現在の状態
};

struct Stg6Raster {
	int x, y;     // 表示座標
	char vy;      //
	uint8_t type; // 種類(0-2)
	uint8_t deg;  // 基準角度
	uint8_t amp;  // 振幅
};

struct Stg6Star {
	int x, y;
	int vy;
};

extern Cube3D Cube[CUBE_MAX];
extern Star2D Star[STAR_MAX];
extern Rock3D Rock[ROCK_MAX];
extern WFLine2D WFLine;
extern FakeECLString FakeECLStr[FAKE_ECLSTR_MAX];
extern Stg6Raster S6Ras[S6RASTER_MAX];
extern Stg6Star S6Star[S3STAR_MAX]; // 兼用モノなのだ


///// [ 関数 ] /////
void Transform3D(Point3D *p, uint8_t dx, uint8_t dy, uint8_t dz);
void DrawLineList3D(std::span<const LineList3D> w);

void DrawWarning(void);
void MoveWarning(uint8_t count);
void MoveWarningR(char count);

void Init3DCube(void);
void Draw3DCube(void);
void Move3DCube(void);

void InitEffectFakeECL(void);
void MoveEffectFakeECL(void);
void DrawEffectFakeECL(void);

void InitStg4Rock(void);	// ステージ４の背景となる岩の集団
void MoveStg4Rock(void);	//
void DrawStg4Rock(void);	//
void SendCmdStg4Rock(uint8_t Cmd, uint8_t Param);	// コマンド送信

void InitStg6Raster();	// ６面ラスター初期化
void MoveStg6Raster();	// ６面ラスター動作
void DrawStg6Raster();	// ６面ラスター描画

void InitStg3Star();	// ３面高速星初期化
void MoveStg3Star();	// ３面高速星動作
void DrawStg3Star();	// ３面高速星描画

#endif
