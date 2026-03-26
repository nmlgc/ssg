/*                                                                           */
/*   LOADER.h   グラフィック、サウンド等のロード                             */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_LOADER_H
#define PBGWIN_LOADER_H		"LOADER : Version 0.10 : Update 2000/02/18"
//#pragma message(PBGWIN_LOADER_H)

#include "constants.h"
#include "hatoyama/engine/graphics.h"
#include "hatoyama/engine/hash.h"
#include "hatoyama/logic/coords.h"


///// [ 定数 ] /////
#define FACE_NUMX				6			// 顔グラの横の個数

// 特殊グラフィックID (For LoadGraph() ) //
#define GRAPH_ID_MUSICROOM		(128)		// 音楽室用ＢＭＰのＩＤ(数値は1-6 で無ければ良い)
#define GRAPH_ID_TITLE			(128+1)		// タイトル画面のＢＭＰのＩＤ
#define GRAPH_ID_NAMEREGIST		(128+2)		// お名前登録画面のＢＭＰのＩＤ
#define GRAPH_ID_EXSTAGE		(128+3)		// エキストラステージ・システム
#define GRAPH_ID_EXBOSS1		(128+4)		// エキストラステージ・ボス１
#define GRAPH_ID_EXBOSS2		(128+5)		// エキストラステージ・ボス２
#define GRAPH_ID_SPROJECT		(128+6)		// 西方Ｐｒｏｊｅｃｔの表示
#define GRAPH_ID_ENDING			(128+7)		// エンディングのロードを行う

// サウンド(効果音番号) //
#define SOUND_ID_KEBARI		0x00
#define SOUND_ID_TAME		0x01
#define SOUND_ID_LASER		0x02
#define SOUND_ID_LASER2		0x03
#define SOUND_ID_BOMB		0x04
#define SOUND_ID_SELECT		0x05
#define SOUND_ID_HIT		0x06
#define SOUND_ID_CANCEL		0x07
#define SOUND_ID_WARNING	0x08
#define SOUND_ID_SBLASER	0x09
#define SOUND_ID_BUZZ		0x0a
#define SOUND_ID_MISSILE	0x0b
#define SOUND_ID_JOINT		0x0c
#define SOUND_ID_DEAD		0x0d
#define SOUND_ID_SBBOMB		0x0e
#define SOUND_ID_BOSSBOMB	0x0f
#define SOUND_ID_ENEMYSHOT	0x10
#define SOUND_ID_HLASER		0x11
#define SOUND_ID_TAMEFAST	0x12
#define SOUND_ID_WARP		0x13

// サウンド(最大数) //
#define SNDMAX_KEBARI		5
#define SNDMAX_TAME			5
#define SNDMAX_LASER		1
#define SNDMAX_LASER2		1
#define SNDMAX_BOMB			1		// 5
#define SNDMAX_SELECT		1
#define SNDMAX_HIT			1		// 5
#define SNDMAX_CANCEL		1
#define SNDMAX_WARNING		1
#define SNDMAX_SBLASER		1
#define SNDMAX_BUZZ			2		// 2
#define SNDMAX_MISSILE		5
#define SNDMAX_JOINT		1
#define SNDMAX_DEAD			1
#define SNDMAX_SBBOMB		1
#define SNDMAX_BOSSBOMB		1
#define SNDMAX_ENEMYSHOT	5
#define SNDMAX_HLASER		1
#define SNDMAX_TAMEFAST		5
#define SNDMAX_WARP			1



///// [ 関数 ] /////
void LoaderInit(void);
void LoaderCleanup(void);
bool LoadStageData(uint8_t stage);	// ＥＣＬ&ＳＣＬデータ列をメモリ上にロードする
BUFFER_OWNED LoadEndingScript(void);
bool LoadGraph(int stage);	// あるステージのグラフィックをロードする
bool LoadFace(uint8_t FaceID, uint8_t FileNo);	// 顔グラフィックをロードする
bool LoadMusic(unsigned int id);	// ｎ番目の曲をロードする
bool LoadMusicByHash(const HASH& hash);
bool LoadMIDIBuffer(BUFFER_OWNED);
bool LoadSound(void);	// 全ての Sound データをロードする

// MusicRoom のコメントをロードする
BUFFER_OWNED LoadMusicRoomComment(int no);

BUFFER_OWNED LoadDemo(int stage);

extern void LoadPaletteFromEnemy(void);		// 敵のパレットにする

// Reloads the last stage loaded with LoadGraph().
void ReloadGraph(void);

//// [ 変数 ] ////
extern uint32_t	MusicNum;	// 曲数

// パレット
extern PALETTE SProjectPalette;
extern PALETTE FacePalette[FACE_MAX]; // 顔グラ用パレット
extern PALETTE EndingPalette[ENDING_PIC_MAX];




#endif
