/*                                                                           */
/*   SCROLL.cpp   スクロール処理                                             */
/*                                                                           */
/*                                                                           */

#include "SCROLL.H"
#include "BOSS.H"
#include "BossHPG.h"
#include "CONFIG.H"
#include "DEMOPLAY.H"
#include "EFFECT.H"
#include "EFFECT3D.H"
#include "ENDING.H"
#include "GAMEMAIN.H"
#include "GIAN.H"
#include "GameDraw.h"
#include "HUD.H"
#include "LOADER.H"
#include "LogicInstance.h"
#include "PRankCtrl.h"
#include "SCL.H" // ＳＣＬ定義ファイル
#include "WindowSys.h"
#include "ssg/internal/RNG.hpp"
#include "ssg/internal/Stage.hpp"
#include "hatoyama/engine/bgm.h"
#include "hatoyama/engine/debug.h"
#include "hatoyama/engine/graphics_backend.h"
#include "hatoyama/engine/snd.h"
#include "hatoyama/logic/cast.h"
#include "hatoyama/logic/endian.h"
#include "hatoyama/logic/ut_math.h"



// マップデータ保存用ヘッダ //
typedef struct tagScrollSaveHeader{
	U32LE	Address;	// このデータの開始アドレス
	U32LE	ScrollWait;	// このレイヤーのディレイ
	U32LE	Length;	// このレイヤーの長さ
} ScrollSaveHeader;

SCROLL_INFO		ScrollInfo;			// スクロールに関する情報
PIXEL_LTRB	rcMapChip[1200];	// マップパーツＩＤに対する矩形

static void InitMapChipRect(void);		// スクロールに関する情報の初期化を行う

static PBGMAP *ScNextLine(PBGMAP *p);		// 次の行にＧＯ！！
static PBGMAP *ScBeforeLine(PBGMAP *p);		// 前の行にＧＯ！！

static void ScrollCmdDummy(void);	// 特殊スクロール無し
static void ScrollCmdStg2Boss(void);	// ２面ボスのスクロール
static void ScrollCmdRasterOpen(void);	// ラスタースクロールオープン
static void ScrollCmdRasterClose(void);	// ラスタースクロールクローズ
static void ScrollCmdStg3Boss(void);	// ゲイツ雲

static void ScrollCmdStg3Star(void);	// ３面高速星

static void Stg3BossMapDraw(void);	// ゲイツ雲描画



// デバッグ用マクロ //
static void SCL_DEBUG(std::u8string_view s)
{
#ifdef SCRIPT_TRACE
	DebugLog(s);
#endif
}



// 背景を動かす(１フレーム分) //
void ScrollMove(void)
{
	int			i;

	ScrollInfo.ExCmd();		// 特殊スクロール発動!!

	// 振動エフェクトを動作させる(これは、特殊スクロールとは別物) //
	if(ScrollInfo.IsQuake) ScrollInfo.IsQuake+=2;

	// 標準のスクロールスピードだけカウンタを進める //
	if(ScrollInfo.DataHead == nullptr) {
		return;
	}
	if(ScrollInfo.Count>=ScrollInfo.InfEnd) return;

	// スクロールしない場合は、リターンする //
	if(ScrollInfo.ScrollSpeed==0) return;

	ScrollInfo.Count += ScrollInfo.ScrollSpeed;

	if(ScrollInfo.ScrollSpeed>0){
		// 通常のスクロール //
		for(i=0;i<ScrollInfo.NumLayer;i++){
			ScrollInfo.LayerCount[i]+= ScrollInfo.ScrollSpeed;
			while(ScrollInfo.LayerCount[i]>=ScrollInfo.LayerWait[i]){
				ScrollInfo.LayerCount[i] -= ScrollInfo.LayerWait[i];
				ScrollInfo.LayerDy[i]  = (ScrollInfo.LayerDy[i]+1)%16;//& 0x0f;
				if(ScrollInfo.LayerDy[i]==0)
					ScrollInfo.LayerPtr[i] = ScNextLine(ScrollInfo.LayerPtr[i]);
			}
		}
	}
	else{
		// 逆方向のスクロール //
		for(i=0;i<ScrollInfo.NumLayer;i++){
			ScrollInfo.LayerCount[i]+= ScrollInfo.ScrollSpeed;
			while(ScrollInfo.LayerCount[i]<0){
				if(ScrollInfo.LayerDy[i]==0)
					ScrollInfo.LayerPtr[i] = ScBeforeLine(ScrollInfo.LayerPtr[i]);
				ScrollInfo.LayerCount[i] += ScrollInfo.LayerWait[i];
				ScrollInfo.LayerDy[i]  = (ScrollInfo.LayerDy[i]+15)%16;//& 0x0f;
			}
		}
	}
}

static PBGMAP *ScNextLine(PBGMAP *p)
{
	int		i;

	for(i=0;i<MAP_WIDTH;){
		if(*p != MAPDATA_NONE) p++,i++;
		else{
			i = i + (*(p+1));
			p = p + 2;
		}
	}

	return p;
}

static PBGMAP *ScBeforeLine(PBGMAP *p)
{
	int		i;

	for(i=0;i<MAP_WIDTH;){
		if(*(p-2) != MAPDATA_NONE) p--,i++;
		else{
			i = i + (*(p-1));
			p = p - 2;
		}
	}

	return p;
}

// p:SCL_ENEMY以降の敵配置データ //
void C_STAGE::Move(INPUT_BITS input)
{
	bool	bFlag = true;
	bool CtrlFlag = false;

	while(bFlag){
		const auto* cmd = SCL_Now;
		switch(cmd[0]){
			case(SCL_KEY):		// キー入力待ち
			break;

			case(SCL_TIME): {
				const auto temp = U32LEAt(&cmd[1]);
				// メッセージウィンドウがオープンしている場合 //
				if(SclInfo.MsgFlag){
					if(!Round.MsgEnabled || (input & KEY_SKIP)) {
						CtrlFlag = true;
					}

					if(CtrlFlag) GameCount += (temp-GameCount)/3;
					else if(input & KEY_RETURN) {
						if(!SclInfo.ReturnFlag){
							GameCount = temp;
							SclInfo.ReturnFlag = true;
						}
					} else {
						SclInfo.ReturnFlag = false;
					}
				}

				if(temp > GameCount) {
					bFlag = false;
				} else {
					SCL_Now += 5;		// cmd(1)+time(4)
				}
				SCL_DEBUG(u8"--- SCL_TIME ---");
			}
			break;

			case(SCL_ENEMY):
				// ボス出現中は出て来ちゃダメ
				if(Boss.NumAlive() == 0) {
					if(auto *e = Enemy.FindFree()) {
						const int16_t x = I16LEAt(&cmd[1 + 0]);
						const int16_t y = I16LEAt(&cmd[1 + 2]);
						const uint8_t id = cmd[1 + 4];
						Enemy.InitEnemyDataSTD(e, x, y, id);
					}
				}
				SCL_Now += 6;	// cmd(1)+x(2)+y(2)+id(1)
				SCL_DEBUG(u8"--- SCL_ENEMY ---");
			break;

			case(SCL_BOSS): { // ボスをセットする(X(16),Y(16),ID(8))
				const auto x  = I16LEAt(&cmd[1 + 0]);	// ボス初期Ｘ
				const auto y  = I16LEAt(&cmd[1 + 2]);	// ボス初期Ｙ
				const auto id = cmd[1 + 2 + 2];	// ボスＩＤ
				Boss.Set(x, y, id);
				BossHPG_Open(Boss.GetHPSum());
				SCL_Now += (1+2+2+1);	// cmd+x+y+id
			}
			break;

			case(SCL_BOSSDEAD):		// ボスを強制的に破壊する(Level2 命令Only)
				Boss.KillAll();
				SCL_Now++;
			break;

			case(SCL_MWOPEN):	// メッセージウィンドウを開く
				if(Round.MsgEnabled) {
					MWinOpen();
				}
				SclInfo.MsgFlag = true;
				SCL_Now++;
			break;

			case(SCL_MWCLOSE):	// メッセージウィンドウを閉じる
				if(Round.MsgEnabled) {
					MWinClose();
				}
				SclInfo.MsgFlag = false;
				SCL_Now++;
			break;

			case(SCL_MSG):		// メッセージを出力する
				MWinMsg(reinterpret_cast<const char *>(cmd + 1));
				SCL_Now += (strlen(reinterpret_cast<const char *>(cmd + 1)) + 2);
			break;

			case(SCL_FACE):		// 顔を表示する
				MWinFace(cmd[1]);
				SCL_Now += 2;
			break;

			case(SCL_LOADFACE):	// 顔グラをロードする(SurfaceID,FileNo)
				LoadFace(cmd[1],cmd[2]);
				SCL_Now += 3;
			break;

			case(SCL_NPG):		// 新しいページに変更する
				MWinCmd(MWCMD_NEWPAGE);
				SCL_Now++;
			break;

			case(SCL_END):		// カウントも変更させずにリターンする
			return;

			case(SCL_SSP):		// スクロールスピード変更
				ScrollSpeed(I16LEAt(&cmd[1]));
				SCL_Now += 3;
			break;

			case(SCL_MUSIC):
//				if(!(/*DemoplaySaveEnable||*/DemoplayLoadEnable)){
				if(!IsDemoplay){
					if(BGM_Switch(cmd[1])){
						const auto mtitle = BGM_Title();
						if(!mtitle.empty()) {
							MTitleSet(460, mtitle);
						}
					}
				}
				SCL_Now += 2;
			break;

			case(SCL_DELENEMY):
				Enemy.IndSet();
				SCL_Now++;
			break;

			case(SCL_EFC):
				switch(cmd[1]){
				case(SEFC_WARN):
					Snd_SEPlay(8, GX_MID, true);
					WarningEffectSet();
				break;

				case(SEFC_WARNSTOP):	Snd_SEStop(8);	break;	// Warning 停止
				case(SEFC_MUSICFADE):	BGM_FadeOut(120);	break;	// 曲の停止
				case(SEFC_STG2BOSS):	ScrollCommand(SCMD_STG2BOSS);	break;	// ２面ボスScroll
				case(SEFC_RASTERON):	ScrollCommand(SCMD_RASTER_ON);	break;	// ラスターON
				case(SEFC_RASTEROFF):	ScrollCommand(SCMD_RASTER_OFF);	break;	// ラスターOFF
				case(SEFC_STG3BOSS):	ScrollCommand(SCMD_STG3BOSS);	break;	// ３面ボス雲
				case(SEFC_STG3RESET):	ScrollCommand(SCMD_STG3RESET);	break;	// ３面リセット
				case(SEFC_CFADEIN):		ScreenEffectSet(SCNEFC_CFADEIN);break;	// ○フェードIn
				case(SEFC_CFADEOUT):	ScreenEffectSet(SCNEFC_CFADEOUT);break;	// ○フェードOut
				case(SEFC_STG6CUBE): // ６面キューブ
					Effect3D.Set(EFFECT3D_TYPE::S6_CUBE);
					ScrollCommand(SCMD_STG6CUBE);
					break;
				case(SEFC_STG6RNDECL): // ６面ＥＣＬ羅列
					Effect3D.Set(EFFECT3D_TYPE::S6_FAKE_ECL);
					ScrollCommand(SCMD_STG6RNDECL);
					break;
				case(SEFC_STG4ROCK): // ４面岩
					Effect3D.Set(EFFECT3D_TYPE::S4_ROCK);
					ScrollCommand(SCMD_STG4ROCK);
					break;
				case(SEFC_STG4LEAVE): // ４面岩画面外へ
					if(Effect3D.GetActive() != EFFECT3D_TYPE::S4_ROCK) {
						break;
					}
					Effect3D.SendCmdStg4Rock(STG4ROCK_LEAVE, 0);
					break;
				case(SEFC_WHITEIN):		ScreenEffectSet(SCNEFC_WHITEIN);break;	// ホワイトイン
				case(SEFC_WHITEOUT):	ScreenEffectSet(SCNEFC_WHITEOUT);break;	// ホワイトアウト
				case(SEFC_LOADEX01):	LoadGraph(GRAPH_ID_EXBOSS1);	break;
				case(SEFC_LOADEX02):	LoadGraph(GRAPH_ID_EXBOSS2);	break;
				case(SEFC_STG6RASTER): // ６面ラスター
					Effect3D.Set(EFFECT3D_TYPE::S6_RASTER);
					ScrollCommand(SCMD_STG6RASTER);
					break;
				}
				SCL_Now += 2;
			break;

			case(SCL_WAITEX):	// 特殊待ち <cmd1>,<opt4>
				switch(cmd[1]){
					case(SWAIT_BOSSHP):		// 残りＨＰ
						if(Boss.GetHPSum() <= U32LEAt(&cmd[2])) {
							break;
						}
					return;
					case(SWAIT_BOSSLEFT):	// 残りボス数
						if(Boss.NumAlive() <= U32LEAt(&cmd[2])) {
							break;
						}
					return;
				}
				SCL_Now += (1+1+4);
			break;

			case(SCL_STAGECLEAR):	// ステージクリア
				if(ConfigDat.StageSelect.v) {
					DemoplaySaveReplay();
					GameExit(true);
					return;
				}
				if(DemoplayLoadEnable) return;
				// ステージクリア処理をここに記述 //
				GameNextStage();	// 本当はエラーチェックが必要!!
			return;

			case(SCL_GAMECLEAR):
				if(ConfigDat.StageSelect.v) {
					DemoplaySaveReplay();
					GameExit(true);
					return;
				}
				if(DemoplayLoadEnable) return;

				if(Round.LevelSelected != GAME_EASY) {
					switch(Viv.weapon){
						case(0):	ConfigDat.ExtraStgFlags.v |= 1;	break;
						case(1):	ConfigDat.ExtraStgFlags.v |= 2;	break;
						case(2):	ConfigDat.ExtraStgFlags.v |= 4;	break;
					}
				}
				ConfigSave();
				EndingInit();
			return;

			case(SCL_EXTRACLEAR):
				if(ConfigDat.StageSelect.v) {
					DemoplaySaveReplay();	// 終了はしない
				}
				if(DemoplayLoadEnable) return;

				NameRegistInit(true);
			return;

			case(SCL_MAPPALETTE):	// マップパーツ用Surface からパレットを
				// 前後にある４０色をマップパーツのパレットにする
				// BitDeapth 判定は、関数側に任せる
				GrpSurface_PaletteApplyToBackend(SURFACE_ID::MAPCHIP);
				SCL_Now++;
			break;

			case(SCL_ENEMYPALETTE):
				LoadPaletteFromEnemy();	// BitDeapth 判定は、関数側に任せる
				SCL_Now++;
			break;

			default:	// 未実装 or ばぐ
				MWinOpen();
				MWinCmd(MWCMD_NEWPAGE);
				MWinCmd(MWCMD_LARGEFONT);
				MWinMsg("バグ発生だにょ");
				SCL_DEBUG(u8"---- SCL !BUG! ---");
			return;
		}
	}

	GameCount++;

	if((GameCount&0x3f)==0){
		PlayRank.Add((GameStage == STAGE_EXTRA) ? 1 : (1 + (GameStage / 3)));
	}
}

// 背景を描画する //
void ScrollDraw(void)
{
	PBGMAP	*p;
	int		i,j,k,x,y;
	int		dx = 0;				// 振動用
	int		RasterDx;			// ラスター用

	if(ScrollInfo.DataHead == nullptr) {
		return;
	}

	// 特殊描画モード //
	if(ScrollInfo.ExCmd == ScrollCmdStg3Boss){
		Stg3BossMapDraw();
		return;
	}
	else if(Effect3D.GetActive() == EFFECT3D_TYPE::S6_CUBE) {
		Draw3DCube(Effect3D);
		return;
	}
	else if(Effect3D.GetActive() == EFFECT3D_TYPE::S6_FAKE_ECL) {
		DrawEffectFakeECL(Effect3D);
		return;
	}
	else if(Effect3D.GetActive() == EFFECT3D_TYPE::S6_RASTER) {
		DrawStg6Raster(Effect3D);
		return;
	}
	else if(ScrollInfo.ExCmd == ScrollCmdStg3Star){
		DrawStg3Star(Effect3D);
		return;
	}

	// 振動エフェクト用 //
	//dx = sinl(ScrollInfo.IsQuake*4,2);
	//if(ScrollInfo.IsQuake) dx = sinl(ScrollInfo.IsQuake*8+i*6,(256-ScrollInfo.IsQuake)>>2);	//4
	if(ScrollInfo.IsQuake) dx = sinl(ScrollInfo.IsQuake*16,(256-ScrollInfo.IsQuake)>>5);	//4

	// 全てのレイヤーの表示 //
	for(k=0;k<ScrollInfo.NumLayer;k++){
		p = ScrollInfo.LayerPtr[k];
		for(i=29;i>=-1;i--){
			RasterDx = (k==0) ? ScrollInfo.RasterDx[i+1] : 0;
			for(j=0;j<MAP_WIDTH;){
				// 通常の描画 //
				if(*p != MAPDATA_NONE){
					x   = (j<<4) + X_MIN + dx + RasterDx;
					y   = (i<<4) + ScrollInfo.LayerDy[k];
					const auto& src = rcMapChip[*p];
					GrpSurface_Blit({ x, y }, SURFACE_ID::MAPCHIP, src);
					p++,j++;
				}
				// 何もない場合 //
				else{
					j = j + (*(p+1));
					p = p + 2;
				}
			}
		}
	}

	if(Effect3D.GetActive() == EFFECT3D_TYPE::S4_ROCK) {
		DrawStg4Rock(Effect3D);
	}
}

// スクロールスピードを変更する(引数:(1)スクロール速度) //
void ScrollSpeed(int speed)
{
	//if(speed<0) speed = 0;		// 逆方向スクロール禁止！！

	ScrollInfo.ScrollSpeed = speed;
}

// ＳＣＬ用コマンド実行関数(引数:(1)スクロールコマンド) //
void ScrollCommand(uint8_t cmd)
{
	switch(cmd){
		case(SCMD_QUAKE):		// 振動エフェクト
			ScrollInfo.IsQuake = 2;
		break;

		case(SCMD_STG2BOSS):	// ２面ボス
			ScrollInfo.ExCmd   = ScrollCmdStg2Boss;
			ScrollInfo.ExCount = 0;
		break;

		case(SCMD_STG3BOSS):	// ３面ボス
			ScrollInfo.ExCmd   = ScrollCmdStg3Boss;
			ScrollInfo.ExCount = 0;
		break;

		case(SCMD_STG3RESET):
		case(SCMD_STG6CUBE):
		case(SCMD_STG6RNDECL):
		case(SCMD_STG4ROCK):
		case(SCMD_STG6RASTER):
			ScrollInfo.ExCmd   = ScrollCmdDummy;
			ScrollInfo.ExCount = 0;
		break;

		case(SCMD_STG3STAR):
			ScrollInfo.ExCmd = ScrollCmdStg3Star;
			ScrollInfo.ExCount = 0;
			ScreenEffectSet(SCNEFC_WHITEIN);
		break;

		case(SCMD_RASTER_ON):	// ラスタースクロール開始
			ScrollInfo.ExCmd       = ScrollCmdRasterOpen;
			ScrollInfo.RasterDeg   = 0;
			ScrollInfo.RasterWidth = 0;
		break;

		case(SCMD_RASTER_OFF):	// ラスタースクロール終了
			ScrollInfo.ExCmd       = ScrollCmdRasterClose;
			//ScrollInfo.RasterDeg   = 0;
			//ScrollInfo.RasterWidth = 0;
		break;
	}
}

// 特殊スクロール無し //
static void ScrollCmdDummy(void)
{
	// 何もしないよぉ... //
}

// ２面ボスのスクロール //
static void ScrollCmdStg2Boss(void)
{
/*
	SSP	-810	TR	10			SSP	-630	TR	10
	SSP	-450	TR	10			SSP	-270	TR	10
	SSP	-180	TR	10			SSP	-90		TR	10
	SSP	0		TR	10			SSP	90		TR	10
	SSP	180		TR	10			SSP	450		TR	10
	SSP	630		TR	10			SSP	810		TR	300
*/
	// 特殊タイマーにより分岐を行う //
	switch(ScrollInfo.ExCount){
		// 正方向->逆方向 //
		case(0):	ScrollSpeed(1512);		break;
		case(20):	ScrollSpeed(1200);		break;
		case(40):	ScrollSpeed(900);		break;
		case(60):	ScrollSpeed(600);		break;
		case(80):	ScrollSpeed(300);		break;
		case(100):	ScrollSpeed(150);		break;
		//case(120):	ScrollSpeed(0);			break;
		case(140):	ScrollSpeed(-150);		break;
		case(160):	ScrollSpeed(-300);		break;
		case(180):	ScrollSpeed(-600);		break;
		case(200):	ScrollSpeed(-900);		break;
		case(220):	ScrollSpeed(-1200);		break;
		case(240):	ScrollSpeed(-1512);		break;

		// 逆方向->正方向 //
		case(440):	ScrollSpeed(-1512);		break;
		case(460):	ScrollSpeed(-1200);		break;
		case(480):	ScrollSpeed(-900);		break;
		case(500):	ScrollSpeed(-600);		break;
		case(520):	ScrollSpeed(-300);		break;
		case(540):	ScrollSpeed(-150);		break;
		//case(560):	ScrollSpeed(0);			break;
		case(580):	ScrollSpeed(150);		break;
		case(600):	ScrollSpeed(300);		break;
		case(620):	ScrollSpeed(600);		break;
		case(640):	ScrollSpeed(900);		break;
		case(660):	ScrollSpeed(1200);		break;
		case(680):	ScrollSpeed(1512);		break;
	}

	ScrollInfo.ExCount = (ScrollInfo.ExCount+1)%880;
}

// ラスタースクロールオープン //
static void ScrollCmdRasterOpen(void)
{
	int		i,j;

	// ちょっと重いかもね... //
	for(i=j=0;i<31;i++,j+=16){
		ScrollInfo.RasterDx[i] = Cast::down<int8_t>(
			sinl((ScrollInfo.RasterDeg + j), ScrollInfo.RasterWidth)
		);
	}

	ScrollInfo.RasterDeg+=2;

	if(ScrollInfo.RasterWidth<2) ScrollInfo.RasterWidth++;
}

// ラスタースクロールクローズ //
static void ScrollCmdRasterClose(void)
{
	int		i,j;

	// ちょっと重いかもね... //
	for(i=j=0;i<31;i++,j+=2){
		ScrollInfo.RasterDx[i] = Cast::down<int8_t>(
			sinl((ScrollInfo.RasterDeg + j), ScrollInfo.RasterWidth)
		);
	}

	ScrollInfo.RasterDeg += 8;

	ScrollInfo.RasterWidth--;

	if(ScrollInfo.RasterWidth==0){
		ScrollInfo.ExCmd = ScrollCmdDummy;
	}
}

// ゲイツ雲 //
static void ScrollCmdStg3Boss(void)
{
	ScrollInfo.ExCount = (ScrollInfo.ExCount+200)%208;
}

// ゲイツ雲描画 //
static void Stg3BossMapDraw(void)
{
	int		x,y;

	x = X_MIN;
	y = Y_MIN - ScrollInfo.ExCount;

	for(; y<Y_MAX; y+=208){
		static const PIXEL_LTRB src = { 0, 272, (640 - 256), (272 + 208) };
		GrpSurface_Blit({ x, y }, SURFACE_ID::MAPCHIP, src);
	}
}


// ３面高速星 //
static void ScrollCmdStg3Star(void)
{
	ScrollInfo.ExCount++;

	if(ScrollInfo.ExCount == 32){
		ScreenEffectSet(SCNEFC_WHITEOUT);
	}
}


// マップデータをロードする //
void ScrollInit(void)
{
	ScrollSaveHeader	*LayerInfo;
	int					i;
	static bool bInitialized = false;

	if(!bInitialized){
		InitMapChipRect();
		bInitialized = true;
	}

	// 標準のスクロールスピード(マップエディタと同じ) //
	auto* head = ScrollInfo.DataHead.get();
	ScrollInfo.ScrollSpeed = TIME_PER_FRAME;					// 標準のスクロール速度
	ScrollInfo.NumLayer    = U32LEAt(head);	// レイヤーの数
	ScrollInfo.Count       = 0;					// スクロール用カウンタ
	ScrollInfo.InfStart    = 0;					// 無限ループ開始時刻
	ScrollInfo.State       = SST_NORMAL;		// 状態(デフォルトの標準で..)
	ScrollInfo.IsQuake     = 0;					// 振動中ではない
	ScrollInfo.ExCmd       = ScrollCmdDummy;	// 特殊コマンド
	ScrollInfo.ExCount     = 0;
	ScrollInfo.RasterWidth = 0;
	ScrollInfo.RasterDeg   = 0;

	// ラスタースクロールの初期化 //
	for(auto& it : ScrollInfo.RasterDx) {
		it = 0;
	}

	// レイヤー情報のロード(長さからループ用変数を調整すること) //
	LayerInfo = reinterpret_cast<ScrollSaveHeader *>(head + sizeof(U32LE));
	for(i=0;i<ScrollInfo.NumLayer;i++){
		auto *p = reinterpret_cast<PBGMAP *>(head + LayerInfo[i].Address);
		ScrollInfo.LayerHead[i]  = p;	// 先頭
		ScrollInfo.LayerPtr[i]   = p;	// 現在
		ScrollInfo.LayerWait[i]  = LayerInfo[i].ScrollWait;		// レイヤーの重み
		ScrollInfo.LayerCount[i] = 0;
		ScrollInfo.LayerDy[i]    = 0;
	}

	// 無限ループ終了時刻 //
	ScrollInfo.InfEnd  = 16*(LayerInfo[i-1].Length-1280/16)*LayerInfo[i-1].ScrollWait;
}

// スクロールに関する情報の初期化を行う //
static void InitMapChipRect(void)
{
	int		i,x,y;

	// マップチップ用矩形の準備 //
	for(i=0;i<1200;i++){
		x = (i%(640/16))<<4;		// マップエディタと同様の演算式
		y = (i/(640/16))<<4;		// マップエディタと同様の演算式
		rcMapChip[i] = { x, y, (x + 16), (y + 16) };
	}
}
