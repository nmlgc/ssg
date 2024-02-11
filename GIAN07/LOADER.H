/*                                                                           */
/*   LOADER.h   グラフィック、サウンド等のロード                             */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_LOADER_H
#define PBGWIN_LOADER_H		"LOADER : Version 0.10 : Update 2000/02/18"
//#pragma message(PBGWIN_LOADER_H)

#include "game/hash.h"
#include "DirectXUTYs/DD_UTY.H"


///// [ 定数 ] /////
#define FACE_MAX				3			// 同時にロード可能な人数...
#define FACE_NUMX				6			// 顔グラの横の個数
constexpr auto ENDING_PIC_MAX = 6;

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



typedef struct tagFACE_DATA{
	PALETTE	pal;	// 顔グラ用パレット
} FACE_DATA;

// エンディングのグラフィック管理用 //
typedef struct tagENDING_GRP{
	PIXEL_LTRB	rcTarget;	// 矩形の範囲
	PALETTE	pal;	// パレット
} ENDING_GRP;



///// [ 関数 ] /////
void LoaderInit(void);
bool LoadStageData(uint8_t stage);	// ＥＣＬ&ＳＣＬデータ列をメモリ上にロードする
bool LoadGraph(int stage);	// あるステージのグラフィックをロードする
bool LoadFace(uint8_t FaceID, uint8_t FileNo);	// 顔グラフィックをロードする
bool LoadMusic(unsigned int id);	// ｎ番目の曲をロードする
bool LoadMusicByHash(const HASH& hash);
bool LoadMIDIBuffer(BYTE_BUFFER_OWNED);
bool LoadSound(void);	// 全ての Sound データをロードする
bool LoadMusicRoomComment(int no);	// MusicRoom のコメントをロードする
BYTE_BUFFER_OWNED LoadDemo(int stage);

extern void LoadPaletteFrom(SURFACE_DDRAW& surf);
extern void LoadPaletteFromEnemy(void);		// 敵のパレットにする


/*
// 廃止：2000/12/01 //
extern void EnterBombPalette(void);		// ボム用パレット属性に変更する
extern void LeaveBombPalette(void);		// ボム用パレット属性を外す
*/

//// [ 変数 ] ////
enum GIAN_SURFACE_ID {
	SURFACE_ID_SYSTEM,	// システム用

	// Title Screen
	SURFACE_ID_TITLE = 2,	// たいとる用

	// Music Room
	SURFACE_ID_MUSIC = 2,	// 音楽室用

	// Name Registration
	SURFACE_ID_NAMEREG = 2,	// お名前登録用

	// In-game
	SURFACE_ID_MAPCHIP = 1,	// 背景用
	SURFACE_ID_ENEMY = 2,	// 敵(雑魚＆ボス)用
	SURFACE_ID_FACE,	// 顔グラ用
	SURFACE_ID_FACE_last = (SURFACE_ID_FACE + FACE_MAX - 1),
	SURFACE_ID_BOMBER,	// ボム用グラフィック用

	// Splash screen
	SURFACE_ID_SPROJECT = 1,	// 西方Project表示用
	SURFACE_ID_LENS = 7,	// レンズエフェクト用

	// Endings
	SURFACE_ID_ENDING_CREDITS = 1,
	SURFACE_ID_ENDING_PIC,
	SURFACE_ID_ENDING_PIC_last = (SURFACE_ID_ENDING_PIC + ENDING_PIC_MAX - 1),

	// Rendered text. Since this one is procedurally generated and therefore
	// doesn't have a palette, it must come last to ensure that DirectDraw
	// initializes it with the implicit palette loaded for an earlier surface.
	SURFACE_ID_TEXT,
};

extern SURFACE_DDRAW&	GrTama;	// システム用
extern SURFACE_DDRAW&	GrEnemy;	// 敵(雑魚＆ボス)用
extern SURFACE_DDRAW&	GrMap;	// 背景用
extern SURFACE_DDRAW&	GrBomber;	// ボム用グラフィック用
extern SURFACE_DDRAW&	GrLens;	// レンズエフェクト用
extern SURFACE_DDRAW&	GrSProject;
extern SURFACE_DDRAW&	GrTitle;
extern SURFACE_DDRAW&	GrMusic;
extern SURFACE_DDRAW&	GrNameReg;
extern SURFACE_DDRAW&	GrEndingCredits;
extern SURFACE_DDRAW&	GrText;

// 顔グラ用
extern const std::reference_wrapper<SURFACE_DDRAW> GrFaces[FACE_MAX];

extern const std::reference_wrapper<SURFACE_DDRAW> GrEndingPic[ENDING_PIC_MAX];

extern FACE_DATA	FaceData[FACE_MAX];	// 顔グラ用

extern uint32_t	MusicNum;	// 曲数

extern PALETTE SProjectPalette;

extern ENDING_GRP	EndingGrp[ENDING_PIC_MAX];



#endif
