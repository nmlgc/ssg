/*                                                                           */
/*   LOADER.cpp   グラフィック、サウンド等のロード                           */
/*                                                                           */
/*                                                                           */

#include "DirectXUTYs/DX_TYPE.H"
#include "game/graphics.h"
#include "ENEMY.H"
#include "CONFIG.H"
#include "GIAN.H"
#include "LOADER.H"
#include "LZ_UTY.H"
#include "LENS.H"
#include "DirectXUTYs/DS_UTY.H"
#include "DirectXUTYs/PBGMIDI.H"
#include <assert.h>
#include <ddraw.h>


// Packfile loading //
bool GrpBMPLoadP(const PACKFILE_READ &in, fil_no_t filno, BYTE surf_id)
{
	auto maybe_bmp = BMPLoad(in.MemExpand(filno));

	// If this fails, we're going to crash due to the uninitialized surface
	// anyway. Might as well announce it in debug mode.
	assert(maybe_bmp);

	// Still necessary to avoid generation of exception handling code in
	// Release mode.
	if(!maybe_bmp) {
		return false;
	}

	auto& bmp = maybe_bmp.value();
	return DxObj.SurfaceLoad(surf_id, std::move(bmp));
}

bool PMid_Load(const PACKFILE_READ &in, fil_no_t filno)
{
	return Mid_Load(in.MemExpand(filno));
}

bool SndWAVLoadP(const PACKFILE_READ &in, fil_no_t filno, BYTE id, int max)
{
	return SndWAVLoad(in.MemExpand(filno), id, max);
}

// Packfile cache //
// -------------- //
// Caching every .DAT file is not only tedious due to the data structures
// involved, but also wrong because replays exist and can be overwritten during
// the lifespan of a process. Since we only actively need these two (plus
// MUSIC.DAT, which needs to be accessible inside LoadMusic()), we can gain
// enough of a load time reduction just by writing two wrapper functions.

BYTE_BUFFER_OWNED DATEnemyLoad(fil_no_t filno)
{
	static const auto in = FilStartR(_PATH("ENEMY.DAT"));
	return in.MemExpand(filno);
}

BYTE_BUFFER_OWNED DATGraphLoad(fil_no_t filno, BYTE surf_id)
{
	static const auto in = FilStartR(_PATH("GRAPH.DAT"));
	return GrpBMPLoadP(in, filno, surf_id);
}
// -------------- //



// グローバル変数 //
SURFACE_DDRAW& GrText	= DxSurf[SURFACE_ID_TEXT];	// 曲のタイトル加工用
SURFACE_DDRAW& GrTama	= DxSurf[SURFACE_ID_SYSTEM];	// 弾など
SURFACE_DDRAW& GrEnemy	= DxSurf[SURFACE_ID_ENEMY];	// 雑魚など
SURFACE_DDRAW& GrMap	= DxSurf[SURFACE_ID_MAPCHIP];	// 背景
SURFACE_DDRAW& GrBomber	= DxSurf[SURFACE_ID_BOMBER];	// ボム用グラフィック用
SURFACE_DDRAW& GrLens	= DxSurf[SURFACE_ID_LENS];	// レンズエフェクト用
SURFACE_DDRAW& GrSProject	= DxSurf[SURFACE_ID_SPROJECT];
SURFACE_DDRAW& GrTitle	= DxSurf[SURFACE_ID_TITLE];
SURFACE_DDRAW& GrMusic	= DxSurf[SURFACE_ID_MUSIC];
SURFACE_DDRAW& GrNameReg	= DxSurf[SURFACE_ID_NAMEREG];

extern const std::reference_wrapper<SURFACE_DDRAW> GrFaces[FACE_MAX] = {
	{ DxSurf[SURFACE_ID_FACE + 0] },
	{ DxSurf[SURFACE_ID_FACE + 1] },
	{ DxSurf[SURFACE_ID_FACE + 2] },
};

extern const std::reference_wrapper<SURFACE_DDRAW> GrEndingPic[
	ENDING_PIC_MAX
] = {
	{ DxSurf[SURFACE_ID_ENEMY] },
	{ DxSurf[SURFACE_ID_ENEMY] },
	{ DxSurf[SURFACE_ID_ENEMY] },
	{ DxSurf[SURFACE_ID_ENEMY] },
	{ DxSurf[SURFACE_ID_BOMBER] },
	{ DxSurf[SURFACE_ID_BOMBER] },
};


LensInfo	*Lens = NULL;

DWORD		MusicNum = 0;			// 曲数
FACE_DATA	FaceData[FACE_MAX];		// 顔グラ用
ENDING_GRP	EndingGrp[ENDING_PIC_MAX];	// エンディング用

char MCommentBuffer[12][50];
char MTitleBuffer[50];




/*
static BOOL			bIsBombPalette = FALSE;
static PALETTEENTRY	tempPalette[256];
*/

static PALETTE EnemyPalette;
PALETTE SProjectPalette;



// 秘密の関数 //
static void SetAnimeRect(ANIME_DATA *anm, int x, int y, int w, int h);
static void SetAnimeRect2(ANIME_DATA *anm, int x1, int y1, int x2, int y2);
static void SetAnimeTH(ANIME_DATA *anm, int x, int y, int n);


// Surface を作成する //
BOOL InitSurface(void)
{
//	bIsBombPalette = FALSE;

	if(
		!DxObj.SurfaceCreateBlank(SURFACE_ID_TEXT, { 640, 20 }) ||
		!DxObj.SurfaceCreateUninitialized(SURFACE_ID_LENS, { 140, 140 })
	) {
		return false;
	}

	// レンズをすでに作成しているのなら、破棄する //
	if(Lens) GrpReleaseLensBall(Lens);

	Lens = GrpCreateLensBall(70, 36, &GrLens);
	if(Lens == NULL) return FALSE;

	return TRUE;
}


//  レンズを破棄する
void CleanupLens(void)
{
	if(Lens) GrpReleaseLensBall(Lens);
}


// あるステージのグラフィックをロードする //
BOOL LoadGraph(int stage)
{
	int	x, y;
	HRESULT		ddrval;

//	bIsBombPalette = FALSE;

	// 音楽室用 //
	if(stage==GRAPH_ID_MUSICROOM){
		if(!DATGraphLoad(0,SURFACE_ID_SYSTEM))		return FALSE;
		if(!DATGraphLoad(19+4,SURFACE_ID_MUSIC))		return FALSE;
		return TRUE;
	}
	// タイトル画面用 //
	if(stage==GRAPH_ID_TITLE){
		if(!DATGraphLoad(0,SURFACE_ID_SYSTEM))		return FALSE;
		if(!DATGraphLoad(20+4,SURFACE_ID_MUSIC))		return FALSE;
		//LoadPaletteFrom(GrEnemy);
		return TRUE;
	}
	// お名前登録画面用 //
	if(stage==GRAPH_ID_NAMEREGIST){
		if(!DATGraphLoad(0,SURFACE_ID_SYSTEM))		return FALSE;
		if(!DATGraphLoad(21+4,SURFACE_ID_NAMEREG))	return FALSE;
		return TRUE;
	}
	// 西方Ｐｒｏｊｅｃｔ表示用 //
	if(stage==GRAPH_ID_SPROJECT){
		if(!DATGraphLoad(31,SURFACE_ID_SPROJECT))		return FALSE;
		GrpGetPalette(SProjectPalette);

		//if(!DATGraphLoad(21+4,SURFACE_ID_NAMEREG))	return FALSE;
		return TRUE;
	}
	// エンディング全画像ロード(パレット含む) //
	if(stage==GRAPH_ID_ENDING){
		const auto in = FilStartR(_PATH("GRAPH2.DAT"));

		RECT src = { 0, 0, 320, 240 };
		for(auto i = 0; i < ENDING_PIC_MAX; i++) {
			if(!GrpBMPLoadP(in,1+i,SURFACE_ID_MAPCHIP))		return FALSE;
			GrpGetPalette(EndingGrp[i].pal);
			if(i>=4) EndingGrp[i].rcTarget = PIXEL_LTWH{ ((i % 4) * 320), 0, 320, 240 };
			else     EndingGrp[i].rcTarget = PIXEL_LTWH{ ((i % 2) * 320), ((i / 2) * 240), 320, 240 };

			x = EndingGrp[i].rcTarget.left;
			y = EndingGrp[i].rcTarget.top;
			for(;;){
				ddrval = GrEndingPic[i].get().surf->BltFast(
					x, y, GrMap.surf, &src, DDBLTFAST_NOCOLORKEY
				);
				if(ddrval == DD_OK){
					break;
				}
				if(ddrval == DDERR_SURFACELOST){
					if(!DxObj.Restore()) {
						return false;
					}
				}
				if(ddrval != DDERR_WASSTILLDRAWING){
					return FALSE;
				}
			}
		}

		if(!GrpBMPLoadP(in, 0, SURFACE_ID_MAPCHIP))		return FALSE;

		return TRUE;
	}


	// エキストラステージシステム用 //
	if(stage == GRAPH_ID_EXSTAGE){
		if(!DATGraphLoad(   0, SURFACE_ID_SYSTEM))	return FALSE;
		if(!DATGraphLoad(27+1, SURFACE_ID_ENEMY))	return FALSE;
		GrpGetPalette(EnemyPalette);

		if(!DATGraphLoad(  27, SURFACE_ID_MAPCHIP))	return FALSE;

		// 諸事情により、ここにいるのです //
		if(!DATGraphLoad(     26,SURFACE_ID_BOMBER))	return FALSE;

		return TRUE;
	}

	// エキストラステージボス用(1) //
	if(stage == GRAPH_ID_EXBOSS1){
		if(!DATGraphLoad(29, SURFACE_ID_ENEMY))		return FALSE;
		GrpGetPalette(EnemyPalette);

		return TRUE;
	}

	// エキストラステージボス用(2) //
	if(stage == GRAPH_ID_EXBOSS2){
		if(!DATGraphLoad(30, SURFACE_ID_ENEMY))		return FALSE;
		GrpGetPalette(EnemyPalette);

		return TRUE;
	}

	if(stage<0 || stage>STAGE_MAX) return FALSE;

	// マップチップのロードは後で変換すること //
	if(!DATGraphLoad(       0,SURFACE_ID_SYSTEM))		return FALSE;
	if(!DATGraphLoad(stage+ 0,SURFACE_ID_ENEMY))		return FALSE;
	GrpGetPalette(EnemyPalette);

	//DWORD MapChipID[6] = {7,7,8,9,10,11};	// 本当は STAGE_MAX とすべき
	DWORD MapChipID[STAGE_MAX] = {7,8,9,10,11,12};
	if(!DATGraphLoad(MapChipID[stage-1],SURFACE_ID_MAPCHIP))	return FALSE;

	// 諸事情により、ここにいるのです //
	if(!DATGraphLoad(     26,SURFACE_ID_BOMBER))		return FALSE;

	return TRUE;
}

bool LoadFace(uint8_t FaceID, uint8_t FileNo)
{
	if(FaceID >= FACE_MAX) {
		return false;
	}
	if(!DATGraphLoad((13 + FileNo), (SURFACE_ID_FACE + FaceID))) {
		return false;
	}

	// パレットを保存する //
	GrpGetPalette(FaceData[FaceID].pal);

	return TRUE;
}

void LoadPaletteFrom(SURFACE_DDRAW& surf)
{
	LPDIRECTDRAWPALETTE		lpdp;

	if(DxObj.PixelFormat.IsChanneled()) return;
	if(surf.surf->GetPalette(&lpdp) != DD_OK) {
		return;
	}

	auto p = DxObj.PaletteForDDraw();
	lpdp->GetEntries(0, 0, p.size(), p.data());
	GrpSetPalette(DxObj.Palette);

	lpdp->Release();
}

// 敵のパレットにする
extern void LoadPaletteFromEnemy(void)
{
	if(DxObj.PixelFormat.IsChanneled()) return;
	GrpSetPalette(EnemyPalette);
	return;
/*
	LPDIRECTDRAWPALETTE		lpdp;

	if(DxObj.PixelFormat.IsChanneled()) return;
	if(!GrMap) return;
	if(GrEnemy->GetPalette(&lpdp) != DD_OK) return;
/*
	if(bIsBombPalette){
		lpdp->GetEntries(0, 0, 256, tempPalette);
	}
	else{
*/
/*
		lpdp->GetEntries(0,0,256,DxObj.pe);
		GrpSetPalette(DxObj.pe);
//	}

	lpdp->Release();
*/
}

// ＥＣＬ&ＳＣＬデータ列をメモリ上にロードする //
BOOL LoadStageData(BYTE stage)
{
	int				i;

	// メモリを解放だ！ //
	SCL_Now = nullptr;
	ECL_Head = nullptr;
	SCL_Head = nullptr;
	ScrollInfo.DataHead = nullptr;

	// エキストラステージシステム用 //
	if(stage == GRAPH_ID_EXSTAGE){
		if((ECL_Head=DATEnemyLoad( 24))==NULL)				return FALSE;	// ECL Load
		if((SCL_Head=DATEnemyLoad( 25))==NULL)				return FALSE;	// SCL Load
		if((ScrollInfo.DataHead=DATEnemyLoad( 26))==NULL)	return FALSE;	// MapData Load
	}
	else if(stage == GRAPH_ID_ENDING){
		if((SCL_Head=DATEnemyLoad( 47))==NULL) return FALSE;	// SCL Load

		SCL_Now   = SCL_Head.get();
		GameCount = 0;
		return TRUE;
	}
	else{
		// 各データをロードする //
		if(stage<1 || stage>STAGE_MAX)								return FALSE;
		if((ECL_Head=DATEnemyLoad(stage+0-1))==NULL)				return FALSE;	// ECL Load
		if((SCL_Head=DATEnemyLoad(stage+6-1))==NULL)				return FALSE;	// SCL Load
		if((ScrollInfo.DataHead=DATEnemyLoad(stage+12-1))==NULL)	return FALSE;	// MapData Load
	}

	// スクロール用変数の初期化 //
	if(!ScrollInit()) return FALSE;

	// 各変数の初期化 //
	SCL_Now   = SCL_Head.get();
	GameCount = 0;

	// アニメーションの準備 //
	switch(stage){
		case(GRAPH_ID_EXSTAGE):		// エキストラステージのグラフィック矩形
			// Extra Boss I //
			// 00 : ■Ａ　0 ～ 3   :  翼無し通常　（10fpp)
			SetAnimeTH(Anime+0, 0, 0, 4);

			// 01 : ■Ｂ　4 ～ 7   :  翼有り通常　（10fpp)
			SetAnimeTH(Anime+1, 320, 0, 4);

			// 02 : ■Ｃ　8 ～ 13  :  翼装着　（翼無し->有り）　（6fpp)
			SetAnimeTH(Anime+2, 0, 80, 6);
			Anime[2].mode = ANM_STOP;

			// 03 : ■Ｄ　14 ～ 15 :  翼有り時攻撃（移動無し）　（6fpp)
			SetAnimeTH(Anime+3, 480, 80, 2);

			// 04 : ■Ｅ　16 ～ 17 :  翼装着時移動（もしくは移動攻撃）左　（6fpp)
			SetAnimeTH(Anime+4, 0, 160, 2);

			// 05 : ■Ｆ　18 ～ 19 :  翼装着時移動（もしくは移動攻撃）右　（6fpp)
			SetAnimeTH(Anime+5, 160, 160, 2);

			// 06 : ■Ｇ　24 ～ 30 :  段階変化　（翼有り->無し）　（6fpp)
			SetAnimeTH(Anime+6, 0, 240, 6);
			Anime[6].mode = ANM_STOP;

			// 07 : ■20 : 通常時ダメージ用マスク　（翼有り、無し兼用）
			SetAnimeTH(Anime+7, 320, 160, 1);

			// 08 : ■21 : 停止攻撃時ダメージ用マスク　
			SetAnimeTH(Anime+8, 400, 160, 1);

			// 09 : ■22 : 移動時（左）ダメージ用マスク　
			SetAnimeTH(Anime+9, 480, 160, 1);

			// 10 : ■23 : 移動時（右）ダメージ用マスク　
			SetAnimeTH(Anime+10, 560, 160, 1);

			SetAnime32(0   , 320+32*0, 11, 4, ANM_NORM);
			SetAnime32(0   , 320+32*1, 12, 4, ANM_NORM);
			SetAnime32(0   , 320+32*2, 13, 4, ANM_NORM);
			SetAnime32(0   , 320+32*3, 14, 4, ANM_NORM);
			SetAnime32(0   , 320+32*4, 15, 4, ANM_NORM);
			SetAnime32(32*4, 320     , 16, 4, ANM_NORM);
			SetAnime32(32*4, 320+32*1, 17, 1, ANM_NORM);

			// Extra Boss II //
			// 18 : ■Ａ : 停止アニメ　（10～12fpp)
			SetAnimeTH(Anime+18, 0, 0, 4);

			// 19 : ■Ｂ : 通常段階攻撃１　（?fpp)
			SetAnimeTH(Anime+19, 320, 0, 4);
			Anime[19].mode = ANM_STOP;

			// 20 : ■Ｃ :  通常段階攻撃２　および、高速移動前溜めポーズ　（6fpp)
			SetAnimeTH(Anime+20, 0, 80, 2);

			// 21 : ■Ｄ : 魂状態（ショットに当たらない無敵） (1 ～ 2 fpp）
			// (160,80), (200,80), (240,80), (280,80)
			Anime[21].size = { 40, 40 };
			Anime[21].n      = 4;
			Anime[21].mode   = ANM_NORM;
			for(i=0; i<4; i++)  Anime[21].ptn[i] = PIXEL_LTWH{ (160 + (i * 40)), 80, 40, 40 };

			// 22 : ■Ｅ : ダメージマスク(A)
			SetAnimeTH(Anime+22, 320, 80, 1);

			// 23 : ■Ｅ : ダメージマスク(B)
			SetAnimeTH(Anime+23, 400, 80, 1);

			// 24 : ■Ｅ : ダメージマスク(G)
			SetAnimeTH(Anime+24, 480, 80, 1);

			// 25 : ■Ｅ : ダメージマスク(C)
			SetAnimeTH(Anime+25, 560, 80, 1);

			// 26 : ■Ｆ : 高速移動アニメ
			Anime[26].size = { 80, 80 };
			Anime[26].n      = 16;
			Anime[26].mode   = ANM_DEG;		// 16 パターンで助かりましたな...
			for(i=0; i<16; i++)
				Anime[26].ptn[i] = PIXEL_LTWH{ ((i * 80) % 640), 160, 80, 80 };

			// 27 : ■Ｇ : 通常段階攻撃２溜めポーズ　および、ワープ前後、
			SetAnimeTH(Anime+27, 560, 320, 1);

			// 28-32 : 陰陽玉ｘ５
			SetAnime32(  0, 384   , 28, 8, ANM_NORM);
			SetAnime32(  0, 384+32, 29, 8, ANM_NORM);
			SetAnime32(  0, 384+64, 30, 8, ANM_NORM);
			SetAnime32(256, 384+32, 31, 8, ANM_NORM);
			SetAnime32(256, 384+64, 32, 8, ANM_NORM);

			SetAnime32(0,  0, 33, 16, ANM_DEG);
			SetAnime32(0, 32, 34, 16, ANM_DEG);
			SetAnime32(0, 64, 35, 16, ANM_DEG);
			SetAnime32(0, 96, 36, 16, ANM_DEG);
			SetAnime32(0,128, 37, 16, ANM_DEG);

			// レーザー発射物 //
			Anime[38].size = { 40, 56 };
			Anime[38].n      = 1;
			Anime[38].mode   = ANM_NORM;
			Anime[38].ptn[0] = PIXEL_LTWH{ 512, 0, 40, 56 };

			// 中ボス //
			Anime[39].size = { 72, 56 };
			Anime[39].n      = 2;
			Anime[39].mode   = ANM_NORM;
			Anime[39].ptn[0] = {  0, 424,  72     , 480 };
			Anime[39].ptn[1] = { 72, 424, (72 * 2), 480 };

			// 中ボスヒット //
			Anime[40].size = { 72, 56 };
			Anime[40].n      = 1;
			Anime[40].mode   = ANM_NORM;
			Anime[40].ptn[0] = { (72 * 2), 424, (72 * 3), 480 };

			// レーザー発射物ヒット //
			Anime[41].size = { 40, 64 };
			Anime[41].n      = 1;
			Anime[41].mode   = ANM_NORM;
			Anime[41].ptn[0] = PIXEL_LTWH{ 512, 56, 40, 56 };

			// 謎の光弾 //
			Anime[42].size = { 24, 24 };
			Anime[42].n      = 4;
			Anime[42].mode   = ANM_NORM;
			Anime[42].ptn[0] = PIXEL_LTWH{ 552,  0, 24, 24 };
			Anime[42].ptn[1] = PIXEL_LTWH{ 552, 24, 24, 24 };
			Anime[42].ptn[2] = PIXEL_LTWH{ 552,  0, 24, 24 };
			Anime[42].ptn[3] = PIXEL_LTWH{ 552, 48, 24, 24 };
		break;

		case(1):		// Ｓｔａｇｅ１のグラフィック矩形
			// 中ボス //
			Anime[0].size = { 72, 56 };
			Anime[0].n      = 2;
			Anime[0].mode   = ANM_NORM;
			Anime[0].ptn[0] = {  0, 0,  72     , 56 };
			Anime[0].ptn[1] = { 72, 0, (72 * 2), 56 };

			SetAnime32(0,56+ 0,1,16,ANM_DEG);
			SetAnime32(0,56+32,2,16,ANM_DEG);
			SetAnime32(0,56+64,3,16,ANM_DEG);
			SetAnime32(0,56+96,4,16,ANM_DEG);

			// ボス //
			Anime[5].size = { 72, 64 };
			Anime[5].n      = 1;
			Anime[5].mode   = ANM_NORM;
			Anime[5].ptn[0] = { 0, 184, 72, 248 };

			// 中ボスフラッシュ用 //
			Anime[6].size = { 72, 56 };
			Anime[6].n      = 2;
			Anime[6].mode   = ANM_NORM;
			Anime[6].ptn[0] = { (72 * 2), 0, (72 * 3), 56 };
			Anime[6].ptn[1] = { (72 * 3), 0, (72 * 4), 56 };

			// ボスフラッシュ //
			Anime[7].size = { 72, 64 };
			Anime[7].n      = 1;
			Anime[7].mode   = ANM_NORM;
			Anime[7].ptn[0] = { 72, 184, (72 * 2), 248 };
		break;

		case(2):		// Ｓｔａｇｅ２のグラフィック矩形
			SetAnime32(0,  0,0,16,ANM_DEG);
			SetAnime32(0, 32,1,16,ANM_DEG);
			SetAnime32(0, 64,2,16,ANM_DEG);
			SetAnime32(0, 96,3,16,ANM_DEG);
			SetAnime32(0,128,4,16,ANM_DEG);

			Anime[5].size = { 112, 48 };
			Anime[5].n      = 1;
			Anime[5].mode   = ANM_NORM;
			Anime[5].ptn[0] = {   0, 160, 112, 208 };

			Anime[6].size = { 64, 48 };
			Anime[6].n      = 1;
			Anime[6].mode   = ANM_NORM;
			Anime[6].ptn[0] = { 112, 160, 176, 208 };

			// 中ボス //
			Anime[7].size = { 64, 64 };
			Anime[7].n      = 1;
			Anime[7].mode   = ANM_NORM;
			Anime[7].ptn[0] = {   0, 208,  64, 272 };

			// ボス羽 //
			Anime[8].size = { 112, 48 };
			Anime[8].n      = 1;
			Anime[8].mode   = ANM_NORM;
			Anime[8].ptn[0] = { 176, 160, 288, 208 };

			// ボス丸 //
			Anime[9].size = { 64, 48 };
			Anime[9].n      = 1;
			Anime[9].mode   = ANM_NORM;
			Anime[9].ptn[0] = { 288, 160, 352, 208 };

			// ボスフラッシュ１ //
			Anime[10].size = { 112, 48 };
			Anime[10].n      = 1;
			Anime[10].mode   = ANM_NORM;
			Anime[10].ptn[0] = { 176, (160 + 48), 288, (208 + 48) };

			// ボスフラッシュ２ //
			Anime[11].size = { 64, 48 };
			Anime[11].n      = 1;
			Anime[11].mode   = ANM_NORM;
			Anime[11].ptn[0] = { 288, (160 + 48), 352, (208 + 48) };

			// 中ボスフラッシュ //
			Anime[12].size = { 64, 64 };
			Anime[12].n      = 1;
			Anime[12].mode   = ANM_NORM;
			Anime[12].ptn[0] = { (0 + 64), 208, (64 + 64), 272 };

/*
			// 羽モノ Left-I //
			Anime[10].size = { 104, 72 };
			Anime[10].n      = 1;
			Anime[10].mode   = ANM_NORM;
			Anime[10].ptn[0] = { 184, 208, 288, 280 };

			// 羽モノ Right-I //
			Anime[11].size = { 104, 72 };
			Anime[11].n      = 1;
			Anime[11].mode   = ANM_NORM;
			Anime[11].ptn[0] = { 288, 208, 392, 280 };

			// 羽モノ Left-0 //
			Anime[12].size = { 88, 80 };
			Anime[12].n      = 1;
			Anime[12].mode   = ANM_NORM;
			Anime[12].ptn[0] = { 200, 280, 288, 360 };

			// 羽モノ Right-0 //
			Anime[13].size = { 88, 80 };
			Anime[13].n      = 1;
			Anime[13].mode   = ANM_NORM;
			Anime[13].ptn[0] = { 288, 280, 376, 360 };
*/
			SetAnimeRect2(Anime+ 14,  0, 288, 159, 479);	// 雲
			SetAnimeRect2(Anime+ 15,160, 384, 271, 479);	//
			SetAnimeRect2(Anime+ 16,272, 368, 390, 478);	//
			SetAnimeRect2(Anime+ 17,400, 368, 496, 431);	//
			SetAnimeRect2(Anime+ 18,400, 160, 558, 359);	//
			SetAnimeRect2(Anime+ 19,528,  48, 639, 160);	//
			SetAnimeRect2(Anime+ 20,560, 160, 639, 270);	//
			SetAnimeRect2(Anime+ 21,576, 320, 639, 399);	//
		break;

		case(3):		// ゲイツ殿のステージ
			Anime[0].size = { 56, 56 };
			Anime[0].n      = 16;
			Anime[0].mode   = ANM_DEG;
			for(i=0;i<8;i++) Anime[0].ptn[i    ] = PIXEL_LTWH{ i*56,  0, 56, 56 };
			for(i=0;i<8;i++) Anime[0].ptn[i + 8] = PIXEL_LTWH{ i*56, 56, 56, 56 };

			SetAnime32(0,112,1,16,ANM_DEG);
			SetAnime32(0,144,2,16,ANM_DEG);
			SetAnime32(0,176,3,16,ANM_DEG);

			Anime[4].size = { 48, 16 };
			Anime[4].n      = 2;
			Anime[4].mode   = ANM_NORM;
			Anime[4].ptn[0] = PIXEL_LTWH{ 592,  0, 48, 16 };
			Anime[4].ptn[1] = PIXEL_LTWH{ 592, 16, 48, 16 };

			Anime[5].size = { 48, 16 };
			Anime[5].n      = 2;
			Anime[5].mode   = ANM_NORM;
			Anime[5].ptn[0] = PIXEL_LTWH{ 592, 32, 48, 16 };
			Anime[5].ptn[1] = PIXEL_LTWH{ 592, 48, 48, 16 };

			// ボス (464,384)
			Anime[6].size = { 11*16, 5*16 + 8 };
			Anime[6].n      = 1;
			Anime[6].mode   = ANM_NORM;
			Anime[6].ptn[0] = PIXEL_LTWH{ 464, 392, (11 * 16), ((5 * 16) + 8) };

			SetAnime32(0,208,7,16,ANM_DEG);
			SetAnime40(0,240,8);

			// ボスの影 //
			Anime[10].size = { 196, 100 };
			Anime[10].n      = 1;
			Anime[10].mode   = ANM_NORM;
			Anime[10].ptn[0] = { 444, 292, 640, 392 };

			// ボスフラッシュ
			Anime[9].size = { 128, 76 };
			Anime[9].n      = 1;
			Anime[9].mode   = ANM_NORM;
			Anime[9].ptn[0] = {  512, 164, 640, 240 };
		/*	Anime[9].size = { (11 * 16), ((5 *16) + 8) };
			Anime[9].n      = 1;
			Anime[9].mode   = ANM_NORM;
			Anime[9].ptn[0] = PIXEL_LTWH{ 464, (392 - 88), (11 * 16), ((5 * 16) + 8) };
		*/
		break;

		case(4):	// マリーさんのステージ
			SetAnime32(0,  0,0,16,ANM_DEG);
			SetAnime32(0, 32,1,16,ANM_DEG);
			SetAnime32(0, 64,2,16,ANM_DEG);
			SetAnime32(0, 96,3, 2,ANM_NORM);
			SetAnime24(64,96,4);
			SetAnime32(0,128,5,16,ANM_DEG);

			//(304,296)-(640,480)
			Anime[6].size = { (640 - 304), (480 - 296) };
			Anime[6].n      = 1;
			Anime[6].mode   = ANM_NORM;
			Anime[6].ptn[0] = { 304, 296, 640, 480 };

			// ボスのフラッシュ //
			Anime[7].size = { (640 - 304 - 32), (480 - 296) };	// ここに注意
			Anime[7].n      = 1;
			Anime[7].mode   = ANM_NORM;
			Anime[7].ptn[0] = {   0, 296, 304, 480 };
		break;

		case(5):	// ご主人様のステージ
			SetAnime32(  0,   0, 0, 16, ANM_DEG);		// 赤いヤツ
			SetAnime32(  0,  32, 1, 16, ANM_DEG);		// 赤いヤツの出現用演出
			SetAnime32(  0,  64, 2, 16, ANM_DEG);		// 蒼いヤツ
			SetAnime32(  0,  96, 3, 16, ANM_DEG);		// 緑のヤツ
			SetAnime32(  0, 128, 4, 16, ANM_DEG);		// オレンジなやつ
			SetAnime32(512,   0, 5,  4, ANM_NORM);		// 原子炉搭載ビット
			SetAnime32(512,  64, 6,  4, ANM_NORM);		// オレンジなヤツの出現用演出

			// 中ボス用のオプション //
			Anime[7].size = { 24, 24 };
			Anime[7].n      = 4;
			Anime[7].mode   = ANM_NORM;
			Anime[7].ptn[0] = PIXEL_LTWH{ 592, (96 +  0), 24, 24 };
			Anime[7].ptn[1] = PIXEL_LTWH{ 592, (96 + 24), 24, 24 };
			Anime[7].ptn[2] = PIXEL_LTWH{ 592, (96 +  0), 24, 24 };
			Anime[7].ptn[3] = PIXEL_LTWH{ 592, (96 + 48), 24, 24 };

			SetAnimeRect(Anime+8, 512, 96, 80, 9*8);	// 屈強なる中ボス

			// メタリックご主人 //
			Anime[9].size = { (640 - 304), (480 - 256) };
			Anime[9].n      = 1;
			Anime[9].mode   = ANM_NORM;
			Anime[9].ptn[0] = { 304, 256, 640, 480 };
		break;

		case(6):
			// ラスボス(座り->立ち) //
			Anime[0].size = { 56, 72 };
			Anime[0].n      = 6;
			Anime[0].mode   = ANM_STOP;
			Anime[0].ptn[0] = PIXEL_LTWH{ (56 * 0), 72, 56, 72 };
			Anime[0].ptn[1] = PIXEL_LTWH{ (56 * 1), 72, 56, 72 };
			Anime[0].ptn[2] = PIXEL_LTWH{ (56 * 2), 72, 56, 72 };
			Anime[0].ptn[3] = PIXEL_LTWH{ (56 * 3), 72, 56, 72 };
			Anime[0].ptn[4] = PIXEL_LTWH{ (56 * 4), 72, 56, 72 };
			Anime[0].ptn[5] = PIXEL_LTWH{ (56 * 5), 72, 56, 72 };

			// ラスボス(立ち->座り) //
			Anime[1].size = { 56, 72 };
			Anime[1].n      = 6;
			Anime[1].mode   = ANM_STOP;
			Anime[1].ptn[0] = PIXEL_LTWH{ (56 * 5), 72, 56, 72 };
			Anime[1].ptn[1] = PIXEL_LTWH{ (56 * 4), 72, 56, 72 };
			Anime[1].ptn[2] = PIXEL_LTWH{ (56 * 3), 72, 56, 72 };
			Anime[1].ptn[3] = PIXEL_LTWH{ (56 * 2), 72, 56, 72 };
			Anime[1].ptn[4] = PIXEL_LTWH{ (56 * 1), 72, 56, 72 };
			Anime[1].ptn[5] = PIXEL_LTWH{ (56 * 0), 72, 56, 72 };

			// ラスボス(ガード) //
			Anime[2].size = { 56, 72 };
			Anime[2].n      = 4;
			Anime[2].mode   = ANM_NORM;
			Anime[2].ptn[0] = PIXEL_LTWH{ (56 * 6), 72, 56, 72 };
			Anime[2].ptn[1] = PIXEL_LTWH{ (56 * 7), 72, 56, 72 };
			Anime[2].ptn[2] = PIXEL_LTWH{ (56 * 6), 72, 56, 72 };
			Anime[2].ptn[3] = PIXEL_LTWH{ (56 * 8), 72, 56, 72 };

			// ラスボス(攻撃-壱) //
			Anime[3].size = { 56, 72 };
			Anime[3].n      = 9+1;
			Anime[3].mode   = ANM_STOP;
			Anime[3].ptn[0] = PIXEL_LTWH{ (56 * 0),  0, 56, 72 };
			Anime[3].ptn[1] = PIXEL_LTWH{ (56 * 1),  0, 56, 72 };
			Anime[3].ptn[2] = PIXEL_LTWH{ (56 * 2),  0, 56, 72 };
			Anime[3].ptn[3] = PIXEL_LTWH{ (56 * 3),  0, 56, 72 };
			Anime[3].ptn[4] = PIXEL_LTWH{ (56 * 4),  0, 56, 72 };
			Anime[3].ptn[5] = PIXEL_LTWH{ (56 * 5),  0, 56, 72 };
			Anime[3].ptn[6] = PIXEL_LTWH{ (56 * 6),  0, 56, 72 };
			Anime[3].ptn[7] = PIXEL_LTWH{ (56 * 7),  0, 56, 72 };
			Anime[3].ptn[8] = PIXEL_LTWH{ (56 * 8),  0, 56, 72 };
			Anime[3].ptn[9] = PIXEL_LTWH{ (56 * 5), 72, 56, 72 };	// ちょっと追加

			// 幼虫期(//
			SetAnimeRect2(Anime+4, 432,272,632,464);

			// ラスボス(ジャンプに見えるといいなぁ) //
			Anime[5].size = { 56, 72 };
			Anime[5].n      = 11;
			Anime[5].mode   = ANM_STOP;
			Anime[5].ptn[ 0] = PIXEL_LTWH{ (56 * 0), 72, 56, 72 };
			Anime[5].ptn[ 1] = PIXEL_LTWH{ (56 * 1), 72, 56, 72 };
			Anime[5].ptn[ 2] = PIXEL_LTWH{ (56 * 2), 72, 56, 72 };
			Anime[5].ptn[ 3] = PIXEL_LTWH{ (56 * 3), 72, 56, 72 };
			Anime[5].ptn[ 4] = PIXEL_LTWH{ (56 * 4), 72, 56, 72 };
			Anime[5].ptn[ 5] = PIXEL_LTWH{ (56 * 5), 72, 56, 72 };
			Anime[5].ptn[ 6] = PIXEL_LTWH{ (56 * 4), 72, 56, 72 };
			Anime[5].ptn[ 7] = PIXEL_LTWH{ (56 * 3), 72, 56, 72 };
			Anime[5].ptn[ 8] = PIXEL_LTWH{ (56 * 2), 72, 56, 72 };
			Anime[5].ptn[ 9] = PIXEL_LTWH{ (56 * 1), 72, 56, 72 };
			Anime[5].ptn[10] = PIXEL_LTWH{ (56 * 0), 72, 56, 72 };


			// 蝶状態で放つビット？(Open) //
			Anime[6].size = { 33, 32 };
			Anime[6].n      = 10;
			Anime[6].mode   = ANM_STOP;
			Anime[6].ptn[0] = PIXEL_LTWH{ (32 * 0), 416, 32, 32 };
			Anime[6].ptn[1] = PIXEL_LTWH{ (32 * 1), 416, 32, 32 };
			Anime[6].ptn[2] = PIXEL_LTWH{ (32 * 2), 416, 32, 32 };
			Anime[6].ptn[3] = PIXEL_LTWH{ (32 * 3), 416, 32, 32 };
			Anime[6].ptn[4] = PIXEL_LTWH{ (32 * 4), 416, 32, 32 };
			Anime[6].ptn[5] = PIXEL_LTWH{ (32 * 5), 416, 32, 32 };
			Anime[6].ptn[6] = PIXEL_LTWH{ (32 * 0), 448, 32, 32 };
			Anime[6].ptn[7] = PIXEL_LTWH{ (32 * 1), 448, 32, 32 };
			Anime[6].ptn[8] = PIXEL_LTWH{ (32 * 2), 448, 32, 32 };
			Anime[6].ptn[9] = PIXEL_LTWH{ (32 * 3), 448, 32, 32 };

			// 蝶状態で放つビット？(Close) //
			Anime[7].size = { 33, 32 };
			Anime[7].n      = 10;
			Anime[7].mode   = ANM_STOP;
			Anime[7].ptn[0] = PIXEL_LTWH{ (32 * 3), 448, 32, 32 };
			Anime[7].ptn[1] = PIXEL_LTWH{ (32 * 2), 448, 32, 32 };
			Anime[7].ptn[2] = PIXEL_LTWH{ (32 * 1), 448, 32, 32 };
			Anime[7].ptn[3] = PIXEL_LTWH{ (32 * 0), 448, 32, 32 };
			Anime[7].ptn[4] = PIXEL_LTWH{ (32 * 5), 416, 32, 32 };
			Anime[7].ptn[5] = PIXEL_LTWH{ (32 * 4), 416, 32, 32 };
			Anime[7].ptn[6] = PIXEL_LTWH{ (32 * 3), 416, 32, 32 };
			Anime[7].ptn[7] = PIXEL_LTWH{ (32 * 2), 416, 32, 32 };
			Anime[7].ptn[8] = PIXEL_LTWH{ (32 * 1), 416, 32, 32 };
			Anime[7].ptn[9] = PIXEL_LTWH{ (32 * 0), 416, 32, 32 };

			SetAnimeRect(Anime+8, 0, 368, 48, 48);	// 屈強なる中ボス
		break;
	}

	return TRUE;
}

// １パターンのグラフィックをアニメとして定義する //
static void SetAnimeRect(ANIME_DATA *anm,int x, int y, int w, int h)
{
	anm->size = { w, h };
	anm->n      = 1;
	anm->mode   = ANM_NORM;

	anm->ptn[0] = PIXEL_LTWH{ x, y, w, h };
}

// １パターンのグラフィックをアニメとして定義する //
static void SetAnimeRect2(ANIME_DATA *anm,int x1, int y1, int x2, int y2)
{
	anm->size = { (x2 - x1), (y2 - y1) };
	anm->n      = 1;
	anm->mode   = ANM_NORM;

	anm->ptn[0] = { x1, y1, x2, y2 };
}

static void SetAnimeTH(ANIME_DATA *anm, int x, int y, int n)
{
	int		i;

	anm->size = { 80, 80 };
	anm->n      = n;
	anm->mode   = ANM_NORM;

	for(i=0; i<n; i++){
		anm->ptn[i] = PIXEL_LTWH{ (x + (i * 80)), y, 80, 80 };
	}
}


// ｎ番目の曲をロードする //
extern BOOL LoadMusic(int no)
{
	BOOL flag;

	static const auto in = FilStartR(_PATH("MUSIC.DAT"));
	flag = PMid_Load(in,no);
	MusicNum = in.info.size();

	return flag;
}

// 全てのSoundデータをロードする //
BOOL LoadSound(void)
{
	const auto in = FilStartR(_PATH("SOUND.DAT"));

	while(1){
		if(!SndWAVLoadP(in,SOUND_ID_KEBARI  ,SOUND_ID_KEBARI  ,SNDMAX_KEBARI  )) break;
		if(!SndWAVLoadP(in,SOUND_ID_TAME    ,SOUND_ID_TAME    ,SNDMAX_TAME    )) break;
		if(!SndWAVLoadP(in,SOUND_ID_LASER   ,SOUND_ID_LASER   ,SNDMAX_LASER   )) break;
		if(!SndWAVLoadP(in,SOUND_ID_LASER2  ,SOUND_ID_LASER2  ,SNDMAX_LASER2  )) break;
		if(!SndWAVLoadP(in,SOUND_ID_BOMB    ,SOUND_ID_BOMB    ,SNDMAX_BOMB    )) break;
		if(!SndWAVLoadP(in,SOUND_ID_SELECT  ,SOUND_ID_SELECT  ,SNDMAX_SELECT  )) break;
		if(!SndWAVLoadP(in,SOUND_ID_HIT     ,SOUND_ID_HIT     ,SNDMAX_HIT     )) break;
		if(!SndWAVLoadP(in,SOUND_ID_CANCEL  ,SOUND_ID_CANCEL  ,SNDMAX_CANCEL  )) break;
		if(!SndWAVLoadP(in,SOUND_ID_WARNING ,SOUND_ID_WARNING ,SNDMAX_WARNING )) break;
		if(!SndWAVLoadP(in,SOUND_ID_SBLASER ,SOUND_ID_SBLASER ,SNDMAX_SBLASER )) break;
		if(!SndWAVLoadP(in,SOUND_ID_BUZZ    ,SOUND_ID_BUZZ    ,SNDMAX_BUZZ))     break;
		if(!SndWAVLoadP(in,SOUND_ID_MISSILE ,SOUND_ID_MISSILE ,SNDMAX_MISSILE )) break;
		if(!SndWAVLoadP(in,SOUND_ID_JOINT   ,SOUND_ID_JOINT   ,SNDMAX_JOINT   )) break;
		if(!SndWAVLoadP(in,SOUND_ID_DEAD    ,SOUND_ID_DEAD    ,SNDMAX_DEAD    )) break;
		if(!SndWAVLoadP(in,SOUND_ID_SBBOMB  ,SOUND_ID_SBBOMB  ,SNDMAX_SBBOMB  )) break;
		if(!SndWAVLoadP(in,SOUND_ID_BOSSBOMB,SOUND_ID_BOSSBOMB,SNDMAX_BOSSBOMB)) break;
		if(!SndWAVLoadP(in,SOUND_ID_ENEMYSHOT,SOUND_ID_ENEMYSHOT,SNDMAX_ENEMYSHOT)) break;
		if(!SndWAVLoadP(in,SOUND_ID_HLASER, SOUND_ID_HLASER, SNDMAX_HLASER))	break;
		if(!SndWAVLoadP(in,SOUND_ID_TAMEFAST, SOUND_ID_TAMEFAST, SNDMAX_TAMEFAST))	break;
		if(!SndWAVLoadP(in,SOUND_ID_WARP, SOUND_ID_WARP, SNDMAX_WARP))	break;
		return TRUE;
	}
	return FALSE;
}


// MusicRoom のコメントをロードする //
extern BOOL LoadMusicRoomComment(int no)
{
	int				i, j;
	BYTE_BUFFER_OWNED	temp;

	if(no < 0 || no > 19) return FALSE;

	if(!(temp=DATEnemyLoad(27+no))) return FALSE;

	for(i=0; i<19*2; i++){
		MTitleBuffer[i] = (BYTE)temp[i];
	}
	MTitleBuffer[19*2] = '\0';

	for(i=0; i<12; i++){
		for(j=0; j<19*2; j++){
			MCommentBuffer[i][j] = (BYTE)temp[(i+1)*19*2 + j];
		}
		MCommentBuffer[i][19*2] = '\0';
	}

	return TRUE;
}


/*
// ボム用パレット属性に変更する //
extern void EnterBombPalette(void)
{
	LPDIRECTDRAWPALETTE		lpdp;

	bIsBombPalette = TRUE;

	if(DxObj.PixelFormat.IsChanneled()) return;
	if(!GrBomber) return;
	if(GrBomber->GetPalette(&lpdp) != DD_OK) return;

	// 現在のパレットを記憶する //
	GrpGetPalette(tempPalette);

	lpdp->GetEntries(0,0,256,DxObj.pe);
	GrpSetPalette(DxObj.pe);

	lpdp->Release();
}


// ボム用パレット属性を外す
extern void LeaveBombPalette(void)
{
	if(!bIsBombPalette) return;

	bIsBombPalette = FALSE;

	GrpSetPalette(tempPalette);
}
*/
