/*                                                                           */
/*   DemoPlay.h   デモプレイ処理                                             */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_DEMOPLAY_H
#define PBGWIN_DEMOPLAY_H		"DEMOPLAY : Version 0.02 : Update 2000/04/01"
//#pragma message(PBGWIN_DEMOPLAY_H)

#include "CONFIG.H"
#include "GIAN.H"


///// [ 定数 ] /////
#define DEMOBUF_MAX		(60*60*30)		// ２０分のデータ格納ＯＫ



///// Replay-specific config option subset /////
// The original code simply reused CONFIG_DATA, which we can't do in this fork
// due to the additional fields we add to the structure.
struct DEMOPLAY_CONFIG_DATA {
	uint8_t GameLevel;
	uint8_t PlayerStock;
	uint8_t BombStock;
	uint8_t Padding1[5] = { 0 };
	uint8_t InputFlags;
	uint8_t Padding2[15] = { 0 };

	void operator =(const CONFIG_DATA& config) {
		GameLevel = config.GameLevel.v;
		PlayerStock = config.PlayerStock.v;
		BombStock = config.BombStock.v;
		InputFlags = config.InputFlags.v;
	}
};
static_assert(sizeof(DEMOPLAY_CONFIG_DATA) == 24);


///// [構造体] /////
typedef struct tagDEMOPLAY_INFO{
	DWORD			RndSeed;		// 乱数のたね
	DWORD			DataSize;		// データ部のサイズ(デコード時に使用する)
	DEMOPLAY_CONFIG_DATA	CfgDat;	// コンフィグの情報(Load時に一部を参照する)
	BYTE			Exp;			// 初期パワーアップ
	BYTE			Weapon;			// 初期装備
} DEMOPLAY_INFO;


///// [ 関数 ] /////
void DemoplayInit(void);	// デモプレイデータの準備
void DemoplayRecord(uint16_t key);	// デモプレイデータを保存する
void DemoplaySave(void);	// デモプレイデータをファイルに書き込む

bool DemoplayLoad(int stage);	// デモプレイデータをロードする
uint16_t DemoplayMove(void);	// Key_Data を返す
void DemoplayLoadCleanup(void);	// デモプレイロードの事後処理


// 要望に付き、追加するのだ //
void ReplayInit(void);	// リプレイデータの準備
void ReplayRecord(uint16_t key);	// リプレイデータを保存する
void ReplaySave(void);	// リプレイデータをファイルに書き込む

bool ReplayLoad(int stage);	// リプレイデータをロードする
uint16_t ReplayMove(void);	// Key_Data を返す
void ReplayLoadCleanup(void);	// リプレイデータロードの事後処理



///// [ 変数 ] /////
extern BOOL DemoplaySaveEnable;		// デモプレイのセーブが動作しているか
extern BOOL DemoplayLoadEnable;		// デモプレイのロードが動作しているか


#endif
