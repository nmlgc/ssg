/*                                                                           */
/*   SCROLL.h   スクロール処理                                               */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_SCROLL_H
#define PBGWIN_SCROLL_H		"SCROLL : Version 0.11 : Update 2000/02/28"
//#pragma message(PBGWIN_SCROLL_H)

#include "platform/buffer.h"


///// [更新履歴] /////
 // 2000/04/01 : スクロールコマンドを追加(STAGE2_BOSS)
 // 2000/02/28 : ＢＯＳＳ命令を Level2 に変更
 // 2000/02/24 : みぢ命令を追加
 // 2000/02/18 : 敵配置処理と結合する
 // 2000/02/01 : 開発開始
 //



///// [ 定数 ] /////

// マップエディタと共有する定数 //
#define LAYER_MAX			5				// レイヤーの深さ
#define TIME_PER_FRAME		20				// １フレームでどれだけ時間が進むか
#define MAP_WIDTH			24				// マップの横幅
#define MAPDATA_NONE		0xffff			// マップ上に何も置かれていない状態

// スクロールステータス //
#define SST_NORMAL			0x00			// 通常時
#define SST_STOP			0x01			// 停止中

// スクロールコマンド //
#define SCMD_QUAKE			0x01			// 画面を振動させる
#define SCMD_STG2BOSS		0x02			// ２面ボスの反転スクロール
#define SCMD_RASTER_ON		0x03			// ラスタースクロール開始
#define SCMD_RASTER_OFF		0x04			// ラスタースクロー終了
#define SCMD_STG3BOSS		0x05			// 背景雲をゲイツモード(謎)に変更する
#define SCMD_STG3RESET		0x06			// ３面の背景をノーマルモードに戻す
#define SCMD_STG6CUBE		0x07			// ６面ボスの３Ｄキューブモード
#define SCMD_STG6RNDECL		0x08			// ６面ボスのランダム偽ＥＣＬ列配置
#define SCMD_STG4ROCK		0x09			// ４面岩
#define SCMD_STG4LEAVE		0x0a			// ４面岩を画面外に吐き出す
#define SCMD_STG6RASTER		0x0b			// ６面ラスター
#define SCMD_STG3STAR		0x0c			// ３面高速星



///// [  型  ] /////
typedef uint16_t	PBGMAP;	// マップパーツ格納用の型



///// [マクロ] /////



///// [構造体] /////

// スクロール管理用構造体 //
typedef struct tagScrollInfo{
//	GRP		lpMapOffs;					// マップパーツデータ(Graphic)の格納先

	BYTE_BUFFER_OWNED	DataHead;					// マップデータのヘッダ

	PBGMAP	*LayerHead[LAYER_MAX];		// 各レイヤーのヘッダ
	PBGMAP	*LayerPtr[LAYER_MAX];		// 現在のレイヤーのポインタ
	uint32_t	LayerWait[LAYER_MAX];	// レイヤーの重み
	int		LayerCount[LAYER_MAX];		// レイヤーごとのカウンタ
	uint8_t	LayerDy[LAYER_MAX];	// レイヤーの1Dot単位のズレ

	int		NumLayer;			// レイヤー数
	int		ScrollSpeed;		// スクロール速度
	uint32_t	Count;	// 現在の時刻
	uint32_t	InfStart;	// 無限ループの開始時刻(デフォルトは０)
	uint32_t	InfEnd;	// 無限ループの終了時刻(デフォルトはマップの長さの最小値)

	uint8_t	State;	// スクロールステータス
	uint8_t	IsQuake;	// 振動中か？

	char	RasterDx[31];		// ラスタースクロールによるＸ加算値
	uint8_t	RasterWidth;	// ラスタースクロールにおける振幅
	uint8_t	RasterDeg;	// ラスタースクロール用の角度

	void(*ExCmd)(void);	// 特殊コマンド
	uint32_t	ExCount;	// 特殊コマンド用カウンタ
} SCROLL_INFO;

// マップデータ保存用ヘッダ //
typedef struct tagScrollSaveHeader{
	uint32_t	Address;	// このデータの開始アドレス
	uint32_t	ScrollWait;	// このレイヤーのディレイ
	uint32_t	Length;	// このレイヤーの長さ
} ScrollSaveHeader;

// ＳＣＬ管理用構造体 //
typedef struct tagSCL_INFO{
	bool	MsgFlag;	// メッセージスキップ用フラグ
	bool	ReturnFlag;	// リターンキー用フラグ
} SCL_INFO;


///// [ 関数 ] /////
void ScrollMove(void);	// 背景を動かす(１フレーム分)
void ScrollDraw(void);	// 背景を描画する

void ScrollSpeed(int speed);	// スクロールスピードを変更する(引数:(1)スクロール速度)
void ScrollCommand(uint8_t cmd);	// ＳＣＬ用コマンド実行関数(引数:(1)スクロールコマンド)

bool ScrollInit(void);	// マップデータを初期化する



///// [ 変数 ] /////
extern SCROLL_INFO		ScrollInfo;			// スクロールに関する情報
extern SCL_INFO			SclInfo;			// ＳＣＬに関する情報



#endif
