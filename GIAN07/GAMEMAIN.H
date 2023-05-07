/*                                                                           */
/*   GameMain.h   ウィンドウシステム切り替えなどの処理                       */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_GAMEMAIN_H
#define PBGWIN_GAMEMAIN_H		"GAMEMAIN : Version 0.01 : Update 2000/02/23"
//#pragma message(PBGWIN_GAMEMAIN_H)



///// [更新履歴] /////

// 2000/02/03 : 製作開始



///// [Include Files] /////
#include "ENDING.H"



///// [ 定数 ] /////
///// [マクロ] /////
///// [構造体] /////

///// [グローバル変数] /////
extern BOOL IsDemoplay;



///// [関数] /////
extern void(*GameMain)(void);	// ゲーム進行用関数ポインタ(WinMainからコールする)

extern bool GameInit(void);	// ゲームの初期化をする
extern void GameRestart(void);	// ゲームを再開する(ESC 抜けから)
extern bool GameExit(bool bNeedChgMusic = true);	// ゲームから抜ける
extern void GameOverInit(void);	// ゲームオーバーの前処理
extern void GameContinue(void);	// コンティニューを行う場合

extern bool GameReplayInit(int Stage);	// リプレイ用の初期化を行う

extern bool SProjectInit(void);	// 西方Ｐｒｏｊｅｃｔ表示の初期化

extern bool GameExstgInit(void);	// エキストラステージを始める

extern bool NameRegistInit(bool bNeedChgMusic);	// お名前入力の初期化
extern bool ScoreNameInit(void);	// お名前表示画面

extern bool GameNextStage(void);	// 次のステージに移行する

extern bool MusicRoomInit(void);	// 音楽室の初期化をする

#endif
