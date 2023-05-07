/*                                                                           */
/*   WINDOWCTRL.cpp   ウィンドウの定義＆管理                                 */
/*                                                                           */
/*                                                                           */

#include "WindowCtrl.h"
#include "WindowSys.h"
#include "CONFIG.H"
#include "GAMEMAIN.H"
#include "LEVEL.H"
#include "LOADER.H"
#include "DirectXUTYs/DD_UTY.H"
#include "DirectXUTYs/DI_UTY.H"
#include "DirectXUTYs/DS_UTY.H"
#include "DirectXUTYs/PBGMIDI.H"



static BOOL DifFnPlayerStock(WORD key);
static BOOL DifFnBombStock(WORD key);
static BOOL DifFnDifficulty(WORD key);

#ifdef PBG_DEBUG
static BOOL DifFnMsgDisplay(WORD key);
static BOOL DifFnStgSelect(WORD key);
static BOOL DifFnHit(WORD key);
static BOOL DifFnDemo(WORD key);
#endif

static BOOL GrpFnChgDevice(WORD key);
static BOOL GrpFnSkip(WORD key);
static BOOL GrpFnBpp(WORD key);
static BOOL GrpFnWinLocate(WORD key);

static BOOL SndFnWAVE(WORD key);
static BOOL SndFnMIDI(WORD key);
static BOOL SndFnMIDIDev(WORD key);

static BOOL InpFnMsgSkip(WORD key);
static BOOL InpFnZSpeedDown(WORD key);

static BOOL InpFnKeyTama(WORD key);
static BOOL InpFnKeyBomb(WORD key);
static BOOL InpFnKeyShift(WORD key);
static BOOL InpFnKeyCancel(WORD key);

static BOOL CfgRepStgSelect(WORD key);
static BOOL CfgRepSave(WORD key);

static BOOL MainFnGameStart(WORD key);
static BOOL MainFnExStart(WORD key);

static BOOL MusicFn(WORD key);

static BOOL ExitFnYes(WORD key);
static BOOL ExitFnNo(WORD key);

static BOOL ContinueFnYes(WORD key);
static BOOL ContinueFnNo(WORD key);

static BOOL ScoreFn(WORD key);

static bool SetDifItem(void);
static bool SetGrpItem(void);
static void SetSndItem(void);
static void SetInpItem(void);
static void SetIKeyItem(void);
static bool SetCfgRepItem(void);

static BOOL RFnStg1(WORD key);
static BOOL RFnStg2(WORD key);
static BOOL RFnStg3(WORD key);
static BOOL RFnStg4(WORD key);
static BOOL RFnStg5(WORD key);
static BOOL RFnStg6(WORD key);
static BOOL RFnStgEx(WORD key);

static bool RingFN(
	bool onchange(void), uint8_t& var, WORD key, uint8_t min, uint8_t max
)
{
	switch(key) {
	case(KEY_BOMB):
	case(KEY_ESC):
		return false;

	case(KEY_RETURN):
	case(KEY_TAMA):
	case(KEY_RIGHT):
		var = ((var == max) ? min : (var + 1));
		break;

	case(KEY_LEFT):
		var = ((var <= min) ? max : (var - 1));
		break;
	}
	return onchange();
}



///// [グローバル変数(公開せず)] /////

char	DifTitle[9][20];
WINDOW_INFO DifItem[9] = {
	{DifTitle[0],"残り人数?を設定します"		,DifFnPlayerStock,0,0},
	{DifTitle[1],"ボムの数を設定します"			,DifFnBombStock,0,0},
	{DifTitle[2],"難易度を設定します"			,DifFnDifficulty,0,0},
#ifdef PBG_DEBUG
	{"-------------------","",0,0,0},
	{DifTitle[4],"[DebugMode] 画面に情報を表示するか"	,DifFnMsgDisplay,0,0},
	{DifTitle[5],"[DebugMode] ステージセレクト"			,DifFnStgSelect,0,0},
	{DifTitle[6],"[DebugMode] 当たり判定"				,DifFnHit,0,0},
	{DifTitle[7],"[DebugMode] デモプレイセーブ"			,DifFnDemo,0,0},
#endif
	{"Exit"		,"一つ前のメニューにもどります"	,CWinExitFn,0,0},
};

char	GrpTitle[5][50];
WINDOW_INFO GrpItem[5] = {
	{GrpTitle[0],"ビデオカードの選択"				,GrpFnChgDevice,0,0},
	{GrpTitle[1],"描画スキップの設定です"			,GrpFnSkip,0,0},
	{GrpTitle[2],"使用する色数を指定します"			,GrpFnBpp,0,0},
	{GrpTitle[3],"ウィンドウの表示位置を決めます"	,GrpFnWinLocate,0,0},
	{"Exit"		,"一つ前のメニューにもどります"		,CWinExitFn,0,0},
};

char	SndTitle[4][20];
WINDOW_INFO SndItem[4] = {
	{SndTitle[0],"WAVEを鳴らすかどうかの設定"	,SndFnWAVE,0,0},
	{SndTitle[1],"MIDIを鳴らすかどうかの設定"	,SndFnMIDI,0,0},
	{SndTitle[2],"MIDI Port (保存はされません)"	,SndFnMIDIDev,0,0},
	{"Exit"		,"一つ前のメニューにもどります"	,CWinExitFn,0,0},
};

char IKeyTitle[4][20];
char InpHelp[34] = "パッド上のボタンを押すと変更";
WINDOW_INFO InpKey[5] = {
	{IKeyTitle[0],InpHelp, InpFnKeyTama,0,0},
	{IKeyTitle[1],InpHelp, InpFnKeyBomb,0,0},
	{IKeyTitle[2],InpHelp, InpFnKeyShift,0,0},
	{IKeyTitle[3],InpHelp, InpFnKeyCancel,0,0},
	{" Exit"		,"一つ前のメニューにもどります"	,CWinExitFn,0,0},
};

char	InpTitle[20];
char	InpTitle2[20];
WINDOW_INFO	InpItem[4] = {
	{InpTitle, "弾キーのメッセージスキップ設定"	,InpFnMsgSkip,0,0},
	{InpTitle2,"弾キーの押しっぱなしで低速移動", InpFnZSpeedDown, 0, 0},
//		8,InpKey,InpKey+1,InpKey+2,InpKey+3,InpKey+4,InpKey+5,InpKey+6,InpKey+7},
	{"Joy Pad"	,"パッドの設定をします"		,0,5,InpKey,InpKey+1,InpKey+2,InpKey+3,InpKey+4},
	{"Exit"		,"一つ前のメニューにもどります"	,CWinExitFn,0,0},
};


char CfgRepTitle[2][20];

WINDOW_INFO CfgRep[3] = {
	{CfgRepTitle[0], "リプレイ用データの保存", CfgRepSave, 0, 0},
	{CfgRepTitle[1], "ステージセレクト"      , CfgRepStgSelect, 0, 0},
	{"Exit"		,"一つ前のメニューにもどります"	,CWinExitFn,0,0},
};

WINDOW_INFO CfgItem[6] = {
	{" Difficulty"	,"難易度に関する設定"
#ifdef PBG_DEBUG
		,0,9,DifItem,DifItem+1,DifItem+2,DifItem+3,DifItem+4,DifItem+5,DifItem+6,DifItem+7,DifItem+8},
#else
		,0,4,DifItem,DifItem+1,DifItem+2,DifItem+3,DifItem+4},
#endif

	{" Graphic"		,"グラフィックに関する設定"		,0,5-1,/*GrpItem,*/GrpItem+1,GrpItem+2,GrpItem+3,GrpItem+4},
	{" Sound / Music"	,"ＳＥ／ＭＩＤＩに関する設定"	,0,4,SndItem,SndItem+1,SndItem+2,SndItem+3},
	{" Input"		,"入力デバイスに関する設定"		,0,4,InpItem,InpItem+1,InpItem+2,InpItem+3},
	{" Replay", "リプレイに関する設定", 0, 3, CfgRep, CfgRep+1, CfgRep+2},
	{" Exit"			,"一つ前のメニューにもどります"	,CWinExitFn,0,0},
};

WINDOW_INFO RepItem[8] = {
	{" Stage 1 デモ再生", "ステージ１のリプレイ", RFnStg1, 0, 0},
	{" Stage 2 デモ再生", "ステージ２のリプレイ", RFnStg2, 0, 0},
	{" Stage 3 デモ再生", "ステージ３のリプレイ", RFnStg3, 0, 0},
	{" Stage 4 デモ再生", "ステージ４のリプレイ", RFnStg4, 0, 0},
	{" Stage 5 デモ再生", "ステージ５のリプレイ", RFnStg5, 0, 0},
	{" Stage 6 デモ再生", "ステージ６のリプレイ", RFnStg6, 0, 0},
	{" ExStage デモ再生", "エキストラステージのリプレイ", RFnStgEx, 0, 0},
	{" Exit"	  ,"一つ前のメニューにもどります"	,CWinExitFn,0,0},
};

WINDOW_INFO MainItem[7] = {
//	{"   Game  Start"	,"ゲームを開始します(使用不可)",0,0,0},
	{"   Game  Start"	,"ゲームを開始します"		,MainFnGameStart,0,0},
	{"   Extra Start"	,"ゲームを開始します(Extra)",MainFnExStart,0,0},
	{"   Replay"		,"リプレイを開始します"		,0,8,RepItem,RepItem+1,RepItem+2,RepItem+3,RepItem+4,RepItem+5,RepItem+6,RepItem+7},
	{"   Config"		,"各種設定を変更します"		,0,6,CfgItem,CfgItem+1,CfgItem+2,CfgItem+3,CfgItem+4,CfgItem+5},
	{"   Score"			,"スコアの表示をします"		,ScoreFn,0,0},
	{"   Music"			,"音楽室に入ります"			,MusicFn,0,0},
//	{"   Exit"			,"ゲームを終了します"		,CWinExitFn,0,0}
	{"   Exit"			,"ゲームを終了します"		,CWinExitFn,0,0}
};

WINDOW_INFO ExitYesNoItem[2] = {
	{"   お っ け ～ ","",ExitFnYes,0,0},
	{"   だ め だ め","",ExitFnNo ,0,0}
};

WINDOW_INFO ContinueYesNoItem[2] = {
	{"   お っ け ～","",ContinueFnYes,0,0},
	{"   や だ や だ","",ContinueFnNo ,0,0}
};


///// [グローバル変数(公開)] /////
WINDOW_SYSTEM MainWindow;
WINDOW_SYSTEM ExitWindow;
WINDOW_SYSTEM ContinueWindow;



// メインメニューの初期化 //
void InitMainWindow(void)
{
	int			i;

	MainWindow.Parent.Title      = "     Main Menu";
	MainWindow.Parent.Help       = " ";			// ここは指定しても意味がない
	MainWindow.Parent.NumItems   = 7;
	MainWindow.Parent.CallBackFn = NULL;

	for(i=0;i<MainWindow.Parent.NumItems;i++){
		MainWindow.Parent.ItemPtr[i] = &MainItem[i];
	}

	SetDifItem();
	SetGrpItem();
	SetSndItem();
	SetInpItem();
	SetIKeyItem();
	SetCfgRepItem();

//	{"   Config"		,"各種設定を変更します"		,0,6,CfgItem,CfgItem+1,CfgItem+2,CfgItem+3,CfgItem+4,CfgItem+5},

	// エキストラステージが選択できる場合には発生！ //
	if(ConfigDat.ExtraStgFlags.v) {
		MainItem[3].NumItems = 6;
		MainItem[3].ItemPtr[4] = CfgItem + 4;
		MainItem[3].ItemPtr[5] = CfgItem + 5;
	}
	else{
		MainItem[3].NumItems = 5;
		MainItem[3].ItemPtr[4] = CfgItem + 5;
	}

	CWinInit(&MainWindow,400,250,0);
	//CWinInit(&MainWindow,150,200,0);
	//CWinInit(&MainWindow,250,150,0);
}


void InitExitWindow(void)
{
	ExitWindow.Parent.Title     = "    終了するの？";
	ExitWindow.Parent.Help      = " ";
	ExitWindow.Parent.NumItems  = 2;
	ExitWindow.Parent.CallBackFn = NULL;

	ExitWindow.Parent.ItemPtr[0] = &ExitYesNoItem[0];
	ExitWindow.Parent.ItemPtr[1] = &ExitYesNoItem[1];

	CWinInit(&ExitWindow,250,150,1);
}

void InitContinueWindow(void)
{
	ContinueWindow.Parent.Title     = " Ｃｏｎｔｉｎｕｅ？";
	ContinueWindow.Parent.Help      = " ";
	ContinueWindow.Parent.NumItems  = 2;
	ContinueWindow.Parent.CallBackFn = NULL;

	ContinueWindow.Parent.ItemPtr[0] = &ContinueYesNoItem[0];
	ContinueWindow.Parent.ItemPtr[1] = &ContinueYesNoItem[1];

	CWinInit(&ContinueWindow,250,150+50,0);
}

static BOOL DifFnPlayerStock(WORD key)
{
	return RingFN(
		SetDifItem, ConfigDat.PlayerStock.v, key, 0, STOCK_PLAYER_MAX
	);
}

static BOOL DifFnBombStock(WORD key)
{
	return RingFN(SetDifItem, ConfigDat.BombStock.v, key, 0, STOCK_BOMB_MAX);
}

static BOOL DifFnDifficulty(WORD key)
{
	return RingFN(
		SetDifItem, ConfigDat.GameLevel.v, key, GAME_EASY, GAME_LUNATIC
	);
}

#ifdef PBG_DEBUG
static BOOL DifFnMsgDisplay(WORD key)
{
	switch(key){
		case(KEY_BOMB):case(KEY_ESC):
		return FALSE;

		case(KEY_RETURN):case(KEY_TAMA):case(KEY_RIGHT):case(KEY_LEFT):
			if(DebugDat.MsgDisplay) DebugDat.MsgDisplay = false;
			else                    DebugDat.MsgDisplay = true;
		break;
	}

	SetDifItem();

	return TRUE;
}

static BOOL DifFnStgSelect(WORD key)
{
	return RingFN(SetDifItem, DebugDat.StgSelect, key, 1, STAGE_MAX);
}

static BOOL DifFnHit(WORD key)
{
	switch(key){
		case(KEY_BOMB):case(KEY_ESC):
		return FALSE;

		case(KEY_RETURN):case(KEY_TAMA):case(KEY_RIGHT):case(KEY_LEFT):
			if(DebugDat.Hit) DebugDat.Hit = false;
			else             DebugDat.Hit = true;
		break;
	}

	SetDifItem();

	return TRUE;
}

static BOOL DifFnDemo(WORD key)
{
	switch(key){
		case(KEY_BOMB):case(KEY_ESC):
		return FALSE;

		case(KEY_RETURN):case(KEY_TAMA):case(KEY_RIGHT):case(KEY_LEFT):
			if(DebugDat.DemoSave) DebugDat.DemoSave = false;
			else                  DebugDat.DemoSave = true;
		break;
	}

	SetDifItem();

	return TRUE;
}

#endif // PBG_DEBUG

static BOOL GrpFnChgDevice(WORD key)
{
	int				flag = 0;

	switch(key){
		case(KEY_BOMB):case(KEY_ESC):
		return FALSE;

		case(KEY_RETURN):case(KEY_TAMA):case(KEY_RIGHT):	flag = 2;
		case(KEY_LEFT):										flag -= 1;

			// 一つしかデバイスが存在しないときは変更できない //
			if(DxEnumNow<=1) break;

			// 次のデバイスへ //
			uint8_t device_id_new = (
				(ConfigDat.DeviceID.v + DxEnumNow + flag) % DxEnumNow
			);

			// この部分に本当ならエラーチェックが必要(後で関数化しろよ) //
			if(DxObj.Init(device_id_new, ConfigDat.BitDepth.v)) {
				ConfigDat.DeviceID.v = device_id_new;
				//GrpSetClip(X_MIN,Y_MIN,X_MAX,Y_MAX);
			}
		break;
	}

	SetGrpItem();

	return TRUE;
}

static BOOL GrpFnSkip(WORD key)
{
	return RingFN(SetGrpItem, ConfigDat.FPSDivisor.v, key, 0, FPS_DIVISOR_MAX);
}

static BOOL GrpFnBpp(WORD key)
{
	switch(key){
		case(KEY_BOMB):case(KEY_ESC):
		return FALSE;

		case(KEY_RETURN):case(KEY_TAMA):case(KEY_RIGHT):case(KEY_LEFT): {
			auto bitdepth_new = ConfigDat.BitDepth.v.cycle(key == KEY_LEFT);

			// この部分に本当ならエラーチェックが必要 //
			if(DxObj.Init(ConfigDat.DeviceID.v, bitdepth_new)) {
				ConfigDat.BitDepth.v = bitdepth_new;
				//GrpSetPalette(DxObj.pe);
				LoadPaletteFrom(GrTitle);

				//GrpSetClip(X_MIN,Y_MIN,X_MAX,Y_MAX);
			}
		break;
		}
	}

	SetGrpItem();

	return TRUE;
}

static BOOL GrpFnWinLocate(WORD key)
{
	BYTE	flags[3] = {0, GRPF_WINDOW_UPPER, GRPF_MSG_DISABLE};
	int		i;

	for(i=0; i<3; i++){
		if(ConfigDat.GraphFlags.v == flags[i]) break;
	}
	if(i >= 3) i=0;

	switch(key){
		case(KEY_BOMB):case(KEY_ESC):
		return FALSE;

		case(KEY_RETURN):case(KEY_TAMA):case(KEY_RIGHT):
			ConfigDat.GraphFlags.v = flags[(i + 1) % 3];
		break;
/*
			if(ConfigDat.GraphFlags.v & GRPF_WINDOW_UPPER) {
				ConfigDat.GraphFlags.v &= (~GRPF_WINDOW_UPPER);
			} else {
				ConfigDat.GraphFlags.v |= GRPF_WINDOW_UPPER;
			}
		break;
*/
		case(KEY_LEFT):
			ConfigDat.GraphFlags.v = flags[(i + 2) % 3];
		break;
	}

	SetGrpItem();

	return TRUE;
}

static BOOL SndFnWAVE(WORD key)
{
	extern HWND hWndMain;

	switch(key){
		case(KEY_BOMB):case(KEY_ESC):
		return FALSE;

		case(KEY_RETURN):case(KEY_TAMA):case(KEY_RIGHT):case(KEY_LEFT):
			//extern INPUT_OBJ InputObj;
			//char buf[100];
			//sprintf(buf,"[1] DI:%x  Dev:%x",InputObj.pdi,InputObj.pdev);
			//DebugOut(buf);

			if(ConfigDat.SoundFlags.v & SNDF_WAVE_ENABLE) {
				ConfigDat.SoundFlags.v &= (~SNDF_WAVE_ENABLE);
				SndCleanup();
			}
			else{
				ConfigDat.SoundFlags.v |= SNDF_WAVE_ENABLE;

				if(!SndInit(hWndMain)) {
					ConfigDat.SoundFlags.v &= (~SNDF_WAVE_ENABLE);
				} else if(!LoadSound()) {
					ConfigDat.SoundFlags.v &= (~SNDF_WAVE_ENABLE);
					SndCleanup();
				}
			}
			//sprintf(buf,"[2] DI:%x  Dev:%x",InputObj.pdi,InputObj.pdev);
			//DebugOut(buf);
		break;
	}

	SetSndItem();

	return TRUE;
}

static BOOL SndFnMIDI(WORD key)
{
	switch(key){
		case(KEY_BOMB):case(KEY_ESC):
		return FALSE;

		case(KEY_RETURN):case(KEY_TAMA):case(KEY_RIGHT):case(KEY_LEFT):
			if(ConfigDat.SoundFlags.v & SNDF_MIDI_ENABLE) {
				Mid_End();
				ConfigDat.SoundFlags.v &= (~SNDF_MIDI_ENABLE);
			}
			else{
				// 成功した場合にだけ有効にする //
				if(Mid_Start(MIDFN_CALLBACK,MIDPL_NORM)){
					ConfigDat.SoundFlags.v |= SNDF_MIDI_ENABLE;
				}
			}
		break;
	}

	SetSndItem();

	return TRUE;
}

static BOOL SndFnMIDIDev(WORD key)
{
	switch(key){
		case(KEY_BOMB):case(KEY_ESC):
		return FALSE;

		case(KEY_RETURN):case(KEY_TAMA):case(KEY_RIGHT):
			if(ConfigDat.SoundFlags.v & SNDF_MIDI_ENABLE) {
				Mid_ChgDev(1);
			}
		break;

		case(KEY_LEFT):
			if(ConfigDat.SoundFlags.v & SNDF_MIDI_ENABLE) {
				Mid_ChgDev(-1);
			}
		break;
	}

	SetSndItem();

	return TRUE;
}

static BOOL InpFnMsgSkip(WORD key)
{
	switch(key){
		case(KEY_BOMB):case(KEY_ESC):
		return FALSE;

		case(KEY_RETURN):case(KEY_TAMA):case(KEY_RIGHT):case(KEY_LEFT):
			if(ConfigDat.InputFlags.v & INPF_Z_MSKIP_ENABLE) {
				ConfigDat.InputFlags.v &= (~INPF_Z_MSKIP_ENABLE);
			} else {
				ConfigDat.InputFlags.v |= INPF_Z_MSKIP_ENABLE;
			}
		break;
	}

	SetInpItem();

	return TRUE;
}

static BOOL InpFnZSpeedDown(WORD key)
{
	switch(key){
		case(KEY_BOMB):case(KEY_ESC):
		return FALSE;

		case(KEY_RETURN):case(KEY_TAMA):case(KEY_RIGHT):case(KEY_LEFT):
			if(ConfigDat.InputFlags.v & INPF_Z_SPDDOWN_ENABLE) {
				ConfigDat.InputFlags.v &= (~INPF_Z_SPDDOWN_ENABLE);
			} else {
				ConfigDat.InputFlags.v |= INPF_Z_SPDDOWN_ENABLE;
			}
		break;
	}

	SetInpItem();

	return TRUE;
}


static bool RFnStg(int stage, WORD key)
{
	if((key == KEY_BOMB) || (key == KEY_ESC)) {
		return false;
	} else if((key == KEY_RETURN) || (key == KEY_TAMA)) {
		GameReplayInit(stage);
	}
	return true;
}


static BOOL RFnStg1(WORD key)
{
	return RFnStg(1, key);
}

static BOOL RFnStg2(WORD key)
{
	return RFnStg(2, key);
}

static BOOL RFnStg3(WORD key)
{
	return RFnStg(3, key);
}

static BOOL RFnStg4(WORD key)
{
	return RFnStg(4, key);
}

static BOOL RFnStg5(WORD key)
{
	return RFnStg(5, key);
}

static BOOL RFnStg6(WORD key)
{
	return RFnStg(6, key);
}

static BOOL RFnStgEx(WORD key)
{
	return RFnStg(GRAPH_ID_EXSTAGE, key);
}



static BOOL MainFnGameStart(WORD key)
{
	switch(key){
		case(KEY_RETURN):case(KEY_TAMA):
			//EndingInit();
			GameInit();
		default:
		return TRUE;
	}
}

static BOOL MainFnExStart(WORD key)
{
	switch(key){
		case(KEY_RETURN):case(KEY_TAMA):
			if(ConfigDat.ExtraStgFlags.v) {
				GameExstgInit();
			}
		default:
		return TRUE;
	}
}

static BOOL ScoreFn(WORD key)
{
	switch(key){
		case(KEY_RETURN):case(KEY_TAMA):
			ScoreNameInit();
		default:
		return TRUE;
	}
}

static BOOL MusicFn(WORD key)
{
	switch(key){
		case(KEY_RETURN):case(KEY_TAMA):
			if(ConfigDat.SoundFlags.v & SNDF_MIDI_ENABLE) {
				MusicRoomInit();
			}
		default:
		return TRUE;
	}
}

static BOOL ExitFnYes(WORD key)
{
	switch(key){
		case(KEY_RETURN):case(KEY_TAMA):
			GameExit();
		return FALSE;

		default:
		return TRUE;
	}
}

static BOOL ExitFnNo(WORD key)
{
	switch(key){
		case(KEY_RETURN):case(KEY_TAMA):
			GameRestart();
		return FALSE;

		default:
		return TRUE;
	}
}

static BOOL ContinueFnYes(WORD key)
{
	switch(key){
		case(KEY_RETURN):case(KEY_TAMA):
			GameContinue();
		return FALSE;

		default:
		return TRUE;
	}
}

static BOOL ContinueFnNo(WORD key)
{
	switch(key){
		case(KEY_RETURN):case(KEY_TAMA):
			NameRegistInit(TRUE);
			//GameExit();
		return FALSE;

		default:
		return TRUE;
	}
}

static bool InpFnKey(uint8_t& config_pad, WORD pad_config_key, WORD key)
{
	key &= (~Pad_Data);
	const auto temp = Key_PadConfig(pad_config_key);
	if(temp >= 0) {
		config_pad = temp;
	}

	SetIKeyItem();

	return ((key != KEY_RETURN) && (key != KEY_TAMA));
}

static BOOL InpFnKeyTama(WORD key)
{
	return InpFnKey(ConfigDat.PadTama.v, KEY_TAMA, key);
}

static BOOL InpFnKeyBomb(WORD key)
{
	return InpFnKey(ConfigDat.PadBomb.v, KEY_BOMB, key);
}

static BOOL InpFnKeyShift(WORD key)
{
	return InpFnKey(ConfigDat.PadShift.v, KEY_SHIFT, key);
}

static BOOL InpFnKeyCancel(WORD key)
{
	return InpFnKey(ConfigDat.PadCancel.v, KEY_ESC, key);
}


static BOOL CfgRepStgSelect(WORD key)
{
	return RingFN(SetCfgRepItem, ConfigDat.StageSelect.v, key, 1, STAGE_MAX);
}


static BOOL CfgRepSave(WORD key)
{
	switch(key){
		case(KEY_BOMB):case(KEY_ESC):
		return FALSE;

		case(KEY_RETURN):case(KEY_TAMA):case(KEY_RIGHT):case(KEY_LEFT):
			ConfigDat.StageSelect.v = ((ConfigDat.StageSelect.v) ? 0 : 1);
		break;
	}

	SetCfgRepItem();

	return TRUE;
}


static bool SetCfgRepItem(void)
{
	const char *SWItem[2]  = {"[ O N ]","[O F F]"};

	if(0 == ConfigDat.StageSelect.v) {
		sprintf(CfgRepTitle[0], "ReplaySave  %s", SWItem[1]);
		strcpy(CfgRepTitle[1], "StageSelect [無 効]");
	}
	else{
		sprintf(CfgRepTitle[0], "ReplaySave  %s", SWItem[0]);
		sprintf(CfgRepTitle[1], "StageSelect [  %d  ]", ConfigDat.StageSelect.v);
	}
	return true;
}


static bool SetDifItem(void)
{
	const char *DifItem[4] = {" Easy  "," Normal"," Hard  ","Lunatic"};
	const char *SWItem[2]  = {"[ O N ]","[O F F]"};
/*
	{DifTitle[4],"[DebugMode] 画面に情報を表示するか"	,DifFnMsgDisplay,0,0},
	{DifTitle[5],"[DebugMode] ステージセレクト"			,DifFnStgSelect,0,0},
	{DifTitle[6],"[DebugMode] 当たり判定"				,DifFnHit,0,0},
*/
	sprintf(DifTitle[0], "PlayerStock [ %d ]", (ConfigDat.PlayerStock.v + 1));	// +1 に注意
	sprintf(DifTitle[1], "BombStock   [ %d ]", ConfigDat.BombStock.v);
	sprintf(DifTitle[2], "Difficulty[%s]", DifItem[ConfigDat.GameLevel.v]);

#ifdef PBG_DEBUG
	sprintf(DifTitle[4],"DebugOut  %s",SWItem[DebugDat.MsgDisplay ? 0 : 1]);
	sprintf(DifTitle[5],"StgSelect [  %d  ]",DebugDat.StgSelect);
	sprintf(DifTitle[6],"Hit       %s",SWItem[DebugDat.Hit ? 0 : 1]);
	sprintf(DifTitle[7],"DemoSave  %s",SWItem[DebugDat.DemoSave ? 0 : 1]);
#endif
	return true;
}

static bool SetGrpItem(void)
{
	const char	*UorD[3]  = {"上のほう","下のほう","描画せず"};
	const char	*DMode[4] = {"おまけ","60Fps","30Fps","20Fps"};
	int		i;

#define SetFlagsMacro(src,flag)		((flag) ? src[0] : src[1])
	sprintf(GrpTitle[0], "Device   [%.7s]", DxEnum[ConfigDat.DeviceID.v].name);
	sprintf(GrpTitle[1], "DrawMode [ %s ]", DMode[ConfigDat.FPSDivisor.v]);
	sprintf(GrpTitle[2], "BitDepth [ %dBit ]", ConfigDat.BitDepth.v.value());
#undef SetFlagsMacro

	if(ConfigDat.GraphFlags.v & GRPF_MSG_DISABLE) {
		i = 2;
	} else {
		i = (ConfigDat.GraphFlags.v & GRPF_WINDOW_UPPER) ? 0 : 1;
	}

	sprintf(GrpTitle[3],"MsgWindow[%s]", UorD[i]);
	return true;
}

static void SetSndItem(void)
{
	const char	*EorD[2] = {" 使用する ","使用しない"};
	static int now;
	char	*ptr,buf[1000];
	int		l;
	static BYTE time = 0;

#define SetFlagsMacro(src,flag)		((flag) ? src[0] : src[1])
	sprintf(SndTitle[0], "WAVE [%s]", SetFlagsMacro(EorD, ConfigDat.SoundFlags.v & SNDF_WAVE_ENABLE));
	sprintf(SndTitle[1], "MIDI [%s]", SetFlagsMacro(EorD, ConfigDat.SoundFlags.v & SNDF_MIDI_ENABLE));

	if(ConfigDat.SoundFlags.v & SNDF_MIDI_ENABLE) {
		time+=16;
		ptr = Mid_Dev.name[Mid_Dev.NowID];
		l = strlen(ptr);
		if(l>18){
			sprintf(buf,"     %s     %s",ptr,ptr);
			if(time==0) now = (now+1)%(l+5);
		}
		else{
			now = 0;
			strcpy(buf,ptr);
		}
		sprintf(SndTitle[2],">%.18s",buf+now);
	}
	else
		sprintf(SndTitle[2],"      ^^^^^^^^^^");
#undef SetFlagsMacro
}

static void SetInpItem(void)
{
	int		temp;

	temp = ((ConfigDat.InputFlags.v & INPF_Z_MSKIP_ENABLE) ? 1 : 0);
	sprintf(InpTitle,"Z-MessageSkip[%s]",temp ? "ＯＫ" : "禁止");

	temp = ((ConfigDat.InputFlags.v & INPF_Z_SPDDOWN_ENABLE) ? 1 : 0);
	sprintf(InpTitle2,"Z-SpeedDown  [%s]",temp ? "ＯＫ" : "禁止");
}

static void SetIKeyItem(void)
{
	sprintf(IKeyTitle[0], "Shot     [Button%2d]", ConfigDat.PadTama.v);
	sprintf(IKeyTitle[1], "Bomb     [Button%2d]", ConfigDat.PadBomb.v);
	sprintf(IKeyTitle[2], "SpeedDown[Button%2d]", ConfigDat.PadShift.v);
	sprintf(IKeyTitle[3], "ESC      [Button%2d]", ConfigDat.PadCancel.v);
}
