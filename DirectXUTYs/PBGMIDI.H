/*                                                                           */
/*   PBGMIDI.h   ＭＩＤＩ管理用関数                                          */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_PBGMIDI_H
#define PBGWIN_PBGMIDI_H	"PBGMIDI : Version 0.31 : Update 2000/08/04"

// 更新履歴 //
// 2000/08/04 : Mid_Free() をグローバルにした。

// 2000/03/22 : MIDI フェードアウト関数をマスターボリュームからＣＣのボリュームに変更
//            : 処理の追いつかない MIDI があった場合は、メッセージ送出に工夫が必要かも




#include "game/narrow.h"
#include "platform/buffer.h"
#include <stdint.h>


enum class MID_BACKEND_STATE : uint8_t {
	STOP,	// 停止している
	PLAY,	// 再生中
	PAUSE,	// 一時停止
};

//// 関数 ////
bool Mid_Start(void);	// ＭＩＤＩ関連初期化
void Mid_End(void);							// ＭＩＤＩ関連おしまい
void Mid_Play(void);						// 再生する
void Mid_Stop(void);						// 停止する
bool Mid_Free(void);	// データ解放する
void Mid_Volume(uint8_t volume);	// マスターボリュームを変更する
void Mid_Tempo(char tempo);					// テンポを変更する
void Mid_FadeOut(uint8_t speed);	// フェードアウト(数字が大きいほど早い)
bool Mid_ChgDev(char pos);	// 出力デバイスを変更する

// Returns the name of the current MIDI device.
Narrow::string_view Mid_DevName(void);

bool Mid_Load(BYTE_BUFFER_OWNED buffer);	// Load a MIDI file from a buffer

char *Mid_GetTitle(void);					// この曲のお名前は？

void Mid_TableInit(void);					// 各種テーブルの初期化



//// グローバル変数 ////
extern uint8_t	Mid_PlayTable[16][128];
extern uint8_t	Mid_PlayTable2[16][128];	// レベルメーター用
extern uint8_t	Mid_NoteTable[16][128];	// ノート表示用
extern uint8_t	Mid_NoteWTable[16][128];	// ノート表示用(2)
extern uint8_t	Mid_PanpodTable[16];	// パンポット
extern uint8_t	Mid_ExpressionTable[16];	// エクスプレッション
extern uint8_t	Mid_VolumeTable[16];	// ボリューム
extern uint32_t	Mid_PlayTime;


#endif
