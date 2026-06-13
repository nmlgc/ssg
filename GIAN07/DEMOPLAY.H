/*                                                                           */
/*   DemoPlay.h   デモプレイ処理                                             */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_DEMOPLAY_H
#define PBGWIN_DEMOPLAY_H		"DEMOPLAY : Version 0.02 : Update 2000/04/01"
//#pragma message(PBGWIN_DEMOPLAY_H)

#include "ssg/Input.h"

struct ROUND_PARAMS;


///// [ 定数 ] /////
#define DEMOBUF_MAX		(60*60*30)		// ２０分のデータ格納ＯＫ



///// [ 関数 ] /////
bool DemoplayIsPlaying(void);

void DemoplayInit(const ROUND_PARAMS& Round); // デモプレイデータの準備

// デモプレイデータを保存する
// Returns `true` if the replay is over and should be saved via either
// DemoplaySave() or ReplaySave().
bool DemoplayRecord(INPUT_BITS key);

void DemoplaySaveDemo(void);	// デモプレイデータをファイルに書き込む
void DemoplaySaveReplay(void);	// リプレイデータをファイルに書き込む

bool DemoplayLoadDemo(uint8_t stage);	// デモプレイデータをロードする
bool DemoplayLoadReplay(uint8_t stage);	// リプレイデータをロードする
INPUT_BITS DemoplayMove(void);	// Key_Data を返す
void DemoplayCleanup(void);	// デモプレイロードの事後処理



///// [ 変数 ] /////
extern bool DemoplaySaveEnable;	// デモプレイのセーブが動作しているか


#endif
