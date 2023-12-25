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
#include "platform/input.h"
#include "platform/midi_backend.h"
#include "platform/input.h"
#include "game/bgm.h"
#include "game/midi.h"
#include "game/snd.h"
#include <numeric>



static bool DifFnPlayerStock(INPUT_BITS key);
static bool DifFnBombStock(INPUT_BITS key);
static bool DifFnDifficulty(INPUT_BITS key);

#ifdef PBG_DEBUG
static bool DifFnMsgDisplay(INPUT_BITS key);
static bool DifFnStgSelect(INPUT_BITS key);
static bool DifFnHit(INPUT_BITS key);
static bool DifFnDemo(INPUT_BITS key);
#endif

static bool GrpFnChgDevice(INPUT_BITS key);
static bool GrpFnSkip(INPUT_BITS key);
static bool GrpFnBpp(INPUT_BITS key);
static bool GrpFnWinLocate(INPUT_BITS key);

static bool SndFnSE(INPUT_BITS key);
static bool SndFnBGM(INPUT_BITS key);
static bool SndFnMIDIDev(INPUT_BITS key);

static bool InpFnMsgSkip(INPUT_BITS key);
static bool InpFnZSpeedDown(INPUT_BITS key);

static bool InpFnKeyTama(INPUT_BITS key);
static bool InpFnKeyBomb(INPUT_BITS key);
static bool InpFnKeyShift(INPUT_BITS key);
static bool InpFnKeyCancel(INPUT_BITS key);

static bool CfgRepStgSelect(INPUT_BITS key);
static bool CfgRepSave(INPUT_BITS key);

static bool MainFnGameStart(INPUT_BITS key);
static bool MainFnExStart(INPUT_BITS key);

static bool MusicFn(INPUT_BITS key);

static bool ExitFnYes(INPUT_BITS key);
static bool ExitFnNo(INPUT_BITS key);

static bool ContinueFnYes(INPUT_BITS key);
static bool ContinueFnNo(INPUT_BITS key);

static bool ScoreFn(INPUT_BITS key);

static void SetDifItem(void);
static void SetGrpItem(void);
static void SetSndItem(void);
static void SetInpItem(void);
static void SetIKeyItem(void);
static void SetCfgRepItem(void);

static bool RFnStg1(INPUT_BITS key);
static bool RFnStg2(INPUT_BITS key);
static bool RFnStg3(INPUT_BITS key);
static bool RFnStg4(INPUT_BITS key);
static bool RFnStg5(INPUT_BITS key);
static bool RFnStg6(INPUT_BITS key);
static bool RFnStgEx(INPUT_BITS key);

static bool RingFN(
	void setitem(void), uint8_t& var, INPUT_BITS key, uint8_t min, uint8_t max
)
{
	return OptionFN(key, setitem, [&]() {
		if(key == KEY_LEFT) {
			var = ((var <= min) ? max : (var - 1));
		} else {
			var = ((var == max) ? min : (var + 1));
		}
	});
}

template <size_t N> struct LABELS {
	const std::array<Narrow::string_view, N> str;
	const size_t w;

	constexpr LABELS(std::array<Narrow::string_view, N> strs) :
		str(strs),
		w(std::reduce(
			strs.begin(), strs.end(), size_t{}, [](auto cur, const auto& str) {
				return (std::max)(cur, str.length());
			}
		)) {
	}
};



///// [グローバル変数(公開せず)] /////
static constexpr const char* CHOICE_OFF_ON[2]  = { "[O F F]", "[ O N ]" };
static constexpr const char* CHOICE_USE[2] = { " 使用する ", "使用しない" };

char	DifTitle[9][20];
WINDOW_INFO DifItem[] = {
	{ DifTitle[0],	"残り人数?を設定します",	DifFnPlayerStock },
	{ DifTitle[1],	"ボムの数を設定します",	DifFnBombStock },
	{ DifTitle[2],	"難易度を設定します",	DifFnDifficulty },
#ifdef PBG_DEBUG
	{ "-------------------" },
	{ DifTitle[4],	"[DebugMode] 画面に情報を表示するか",	DifFnMsgDisplay },
	{ DifTitle[5],	"[DebugMode] ステージセレクト",	DifFnStgSelect },
	{ DifTitle[6],	"[DebugMode] 当たり判定",	DifFnHit },
	{ DifTitle[7],	"[DebugMode] デモプレイセーブ",	DifFnDemo },
#endif
	{ "Exit",	"一つ前のメニューにもどります",	CWinExitFn },
};

char	GrpTitle[5][50];
WINDOW_INFO GrpItem[] = {
	// { GrpTitle[0],	"ビデオカードの選択",	GrpFnChgDevice },
	{ GrpTitle[1],	"描画スキップの設定です",	GrpFnSkip },
	{ GrpTitle[2],	"使用する色数を指定します",	GrpFnBpp },
	{ GrpTitle[3],	"ウィンドウの表示位置を決めます",	GrpFnWinLocate },
	{ "Exit",	"一つ前のメニューにもどります",	CWinExitFn },
};

char	SndTitle[4][26];
WINDOW_INFO SndItem[] = {
	{ SndTitle[0],	"SEを鳴らすかどうかの設定",	SndFnSE },
	{ SndTitle[1],	"BGMを鳴らすかどうかの設定",	SndFnBGM },
	{ SndTitle[2],	"MIDI Port (保存はされません)",	SndFnMIDIDev },
	{ "Exit",	"一つ前のメニューにもどります",	CWinExitFn },
};

char IKeyTitle[4][20];
char InpHelp[] = "パッド上のボタンを押すと変更";
WINDOW_INFO InpKey[] = {
	{ IKeyTitle[0],	InpHelp,	InpFnKeyTama },
	{ IKeyTitle[1],	InpHelp,	InpFnKeyBomb },
	{ IKeyTitle[2],	InpHelp,	InpFnKeyShift },
	{ IKeyTitle[3],	InpHelp,	InpFnKeyCancel },
	{ " Exit",	"一つ前のメニューにもどります",	CWinExitFn },
};

char	InpTitle[23];
char	InpTitle2[23];
WINDOW_INFO InpItem[] = {
	{ InpTitle,	"弾キーのメッセージスキップ設定",	InpFnMsgSkip },
	{ InpTitle2,	"弾キーの押しっぱなしで低速移動",	InpFnZSpeedDown },
	{ "Joy Pad",	"パッドの設定をします",	InpKey },
	{ "Exit",	"一つ前のメニューにもどります",	CWinExitFn },
};


char CfgRepTitle[2][23];

WINDOW_INFO CfgRep[] = {
	{ CfgRepTitle[0],	"リプレイ用データの保存",	CfgRepSave },
	{ CfgRepTitle[1],	"ステージセレクト",	CfgRepStgSelect },
	{ "Exit",	"一つ前のメニューにもどります",	CWinExitFn },
};

WINDOW_INFO CfgItem[] = {
	{ " Difficulty",	"難易度に関する設定",	DifItem },
	{ " Graphic",	"グラフィックに関する設定",	GrpItem },
	{ " Sound / Music",	"ＳＥ／ＢＧＭに関する設定",	SndItem },
	{ " Input",	"入力デバイスに関する設定",	InpItem },
	{ " Replay",	"リプレイに関する設定",	CfgRep },
	{ " Exit",	"一つ前のメニューにもどります",	CWinExitFn },
};

WINDOW_INFO RepItem[] = {
	{ " Stage 1 デモ再生",	"ステージ１のリプレイ",	RFnStg1 },
	{ " Stage 2 デモ再生",	"ステージ２のリプレイ",	RFnStg2 },
	{ " Stage 3 デモ再生",	"ステージ３のリプレイ",	RFnStg3 },
	{ " Stage 4 デモ再生",	"ステージ４のリプレイ",	RFnStg4 },
	{ " Stage 5 デモ再生",	"ステージ５のリプレイ",	RFnStg5 },
	{ " Stage 6 デモ再生",	"ステージ６のリプレイ",	RFnStg6 },
	{ " ExStage デモ再生",	"エキストラステージのリプレイ",	RFnStgEx },
	{ " Exit",	"一つ前のメニューにもどります",	CWinExitFn },
};

WINDOW_INFO MainItem[] = {
	// { "   Game  Start",	"ゲームを開始します(使用不可)" },
	{ "   Game  Start",	"ゲームを開始します",	MainFnGameStart },
	{ "   Extra Start",	"ゲームを開始します(Extra)",	MainFnExStart },
	{ "   Replay",	"リプレイを開始します",	RepItem },
	{ "   Config",	"各種設定を変更します",	CfgItem },
	{ "   Score",	"スコアの表示をします",	ScoreFn },
	{ "   Music",	"音楽室に入ります",	MusicFn },
	{ "   Exit",	"ゲームを終了します",	CWinExitFn }
};

WINDOW_INFO ExitYesNoItem[] = {
	{ "   お っ け ～ ",	"",	ExitFnYes },
	{ "   だ め だ め",	"",	ExitFnNo }
};

WINDOW_INFO ContinueYesNoItem[] = {
	{ "   お っ け ～",	"",	ContinueFnYes },
	{ "   や だ や だ",	"",	ContinueFnNo }
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

	//	{ "   Config",	"各種設定を変更します",	CfgItem },

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

	MainWindow.Init(140);
}


void InitExitWindow(void)
{
	ExitWindow.Parent.Title     = "    終了するの？";
	ExitWindow.Parent.Help      = " ";
	ExitWindow.Parent.NumItems  = 2;
	ExitWindow.Parent.CallBackFn = NULL;

	ExitWindow.Parent.ItemPtr[0] = &ExitYesNoItem[0];
	ExitWindow.Parent.ItemPtr[1] = &ExitYesNoItem[1];

	ExitWindow.Init(140);
}

void InitContinueWindow(void)
{
	ContinueWindow.Parent.Title     = " Ｃｏｎｔｉｎｕｅ？";
	ContinueWindow.Parent.Help      = " ";
	ContinueWindow.Parent.NumItems  = 2;
	ContinueWindow.Parent.CallBackFn = NULL;

	ContinueWindow.Parent.ItemPtr[0] = &ContinueYesNoItem[0];
	ContinueWindow.Parent.ItemPtr[1] = &ContinueYesNoItem[1];

	ContinueWindow.Init(140);
}

static bool DifFnPlayerStock(INPUT_BITS key)
{
	return RingFN(
		SetDifItem, ConfigDat.PlayerStock.v, key, 0, STOCK_PLAYER_MAX
	);
}

static bool DifFnBombStock(INPUT_BITS key)
{
	return RingFN(SetDifItem, ConfigDat.BombStock.v, key, 0, STOCK_BOMB_MAX);
}

static bool DifFnDifficulty(INPUT_BITS key)
{
	return RingFN(
		SetDifItem, ConfigDat.GameLevel.v, key, GAME_EASY, GAME_LUNATIC
	);
}

#ifdef PBG_DEBUG
static bool DifFnMsgDisplay(INPUT_BITS key)
{
	return OptionFN(key, SetDifItem, [] {
		DebugDat.MsgDisplay = !DebugDat.MsgDisplay;
	});
}

static bool DifFnStgSelect(INPUT_BITS key)
{
	return RingFN(SetDifItem, DebugDat.StgSelect, key, 1, STAGE_MAX);
}

static bool DifFnHit(INPUT_BITS key)
{
	return OptionFN(key, SetDifItem, [] {
		DebugDat.Hit = !DebugDat.Hit;
	});
}

static bool DifFnDemo(INPUT_BITS key)
{
	return OptionFN(key, SetDifItem, [] {
		DebugDat.DemoSave = !DebugDat.DemoSave;
	});
}

#endif // PBG_DEBUG

static bool GrpFnChgDevice(INPUT_BITS key)
{
	return OptionFN(key, SetGrpItem, [&]() {
		int flag = ((key == KEY_LEFT) ? -1 : 2);
		// 一つしかデバイスが存在しないときは変更できない //
		if(DxEnumNow <= 1) {
			return;
		}

		// 次のデバイスへ //
		uint8_t device_id_new = (
			(ConfigDat.DeviceID.v + DxEnumNow + flag) % DxEnumNow
		);

		// この部分に本当ならエラーチェックが必要(後で関数化しろよ) //
		TextObj.WipeBeforeNextRender();
		if(DxObj.Init(device_id_new, ConfigDat.BitDepth.v)) {
			ConfigDat.DeviceID.v = device_id_new;
			// GrpSetClip(X_MIN,Y_MIN,X_MAX,Y_MAX);
		}
	});
}

static bool GrpFnSkip(INPUT_BITS key)
{
	return RingFN(SetGrpItem, ConfigDat.FPSDivisor.v, key, 0, FPS_DIVISOR_MAX);
}

static bool GrpFnBpp(INPUT_BITS key)
{
	return OptionFN(key, SetGrpItem, [&]() {
		auto bitdepth_new = ConfigDat.BitDepth.v.cycle(key == KEY_LEFT);

		// この部分に本当ならエラーチェックが必要 //
		TextObj.WipeBeforeNextRender();
		if(DxObj.Init(ConfigDat.DeviceID.v, bitdepth_new)) {
			ConfigDat.BitDepth.v = bitdepth_new;
			// GrpSetPalette(DxObj.pe);
			LoadPaletteFrom(GrTitle);

			// GrpSetClip(X_MIN,Y_MIN,X_MAX,Y_MAX);
		}
	});
}

static bool GrpFnWinLocate(INPUT_BITS key)
{
	BYTE	flags[3] = {0, GRPF_WINDOW_UPPER, GRPF_MSG_DISABLE};
	int		i;

	for(i=0; i<3; i++){
		if(ConfigDat.GraphFlags.v == flags[i]) break;
	}
	if(i >= 3) i=0;

	return OptionFN(key, SetGrpItem, [&] {
		if(key == KEY_LEFT) {
			ConfigDat.GraphFlags.v = flags[(i + 2) % 3];
		} else {
			ConfigDat.GraphFlags.v = flags[(i + 1) % 3];
		}
	});
}

static bool SndFnSE(INPUT_BITS key)
{
	return OptionFN(key, SetSndItem, [] {
		//extern INPUT_OBJ InputObj;
		//char buf[100];
		//sprintf(buf,"[1] DI:%x  Dev:%x",InputObj.pdi,InputObj.pdev);
		// DebugOut(buf);

		if(ConfigDat.SoundFlags.v & SNDF_SE_ENABLE) {
			ConfigDat.SoundFlags.v &= (~SNDF_SE_ENABLE);
			Snd_SECleanup();
		} else {
			ConfigDat.SoundFlags.v |= SNDF_SE_ENABLE;
			LoadSound();
		}
		//sprintf(buf,"[2] DI:%x  Dev:%x",InputObj.pdi,InputObj.pdev);
		// DebugOut(buf);
	});
}

static bool SndFnBGM(INPUT_BITS key)
{
	return OptionFN(key, SetSndItem, [] {
		if(BGM_Enabled()) {
			BGM_Cleanup();
		} else {
			// 成功した場合にだけ有効にする //
			if(BGM_Init()) {
				BGM_Switch(0);
			}
		}
	});
}

static bool SndFnMIDIDev(INPUT_BITS key)
{
	return OptionFN(key, SetSndItem, [&] {
		if(BGM_Enabled()) {
			BGM_ChangeMIDIDevice((key == KEY_LEFT) ? -1 : 1);
		}
	});
}

static bool InpFnMsgSkip(INPUT_BITS key)
{
	return OptionFN(key, SetInpItem, [] {
		if(ConfigDat.InputFlags.v & INPF_Z_MSKIP_ENABLE) {
			ConfigDat.InputFlags.v &= (~INPF_Z_MSKIP_ENABLE);
		} else {
			ConfigDat.InputFlags.v |= INPF_Z_MSKIP_ENABLE;
		}
	});
}

static bool InpFnZSpeedDown(INPUT_BITS key)
{
	return OptionFN(key, SetInpItem, [] {
		if(ConfigDat.InputFlags.v & INPF_Z_SPDDOWN_ENABLE) {
			ConfigDat.InputFlags.v &= (~INPF_Z_SPDDOWN_ENABLE);
		} else {
			ConfigDat.InputFlags.v |= INPF_Z_SPDDOWN_ENABLE;
		}
	});
}


static bool RFnStg(int stage, INPUT_BITS key)
{
	if((key == KEY_BOMB) || (key == KEY_ESC)) {
		return false;
	} else if((key == KEY_RETURN) || (key == KEY_TAMA)) {
		GameReplayInit(stage);
	}
	return true;
}


static bool RFnStg1(INPUT_BITS key)
{
	return RFnStg(1, key);
}

static bool RFnStg2(INPUT_BITS key)
{
	return RFnStg(2, key);
}

static bool RFnStg3(INPUT_BITS key)
{
	return RFnStg(3, key);
}

static bool RFnStg4(INPUT_BITS key)
{
	return RFnStg(4, key);
}

static bool RFnStg5(INPUT_BITS key)
{
	return RFnStg(5, key);
}

static bool RFnStg6(INPUT_BITS key)
{
	return RFnStg(6, key);
}

static bool RFnStgEx(INPUT_BITS key)
{
	return RFnStg(GRAPH_ID_EXSTAGE, key);
}



static bool MainFnGameStart(INPUT_BITS key)
{
	switch(key){
		case(KEY_RETURN):case(KEY_TAMA):
			//EndingInit();
			WeaponSelectInit(false);
		default:
		return TRUE;
	}
}

static bool MainFnExStart(INPUT_BITS key)
{
	switch(key){
		case(KEY_RETURN):case(KEY_TAMA):
			if(ConfigDat.ExtraStgFlags.v) {
				WeaponSelectInit(true);
			}
		default:
		return TRUE;
	}
}

static bool ScoreFn(INPUT_BITS key)
{
	switch(key){
		case(KEY_RETURN):case(KEY_TAMA):
			ScoreNameInit();
		default:
		return TRUE;
	}
}

static bool MusicFn(INPUT_BITS key)
{
	switch(key){
		case(KEY_RETURN):case(KEY_TAMA):
			if(BGM_Enabled()) {
				MusicRoomInit();
			}
		default:
		return TRUE;
	}
}

static bool ExitFnYes(INPUT_BITS key)
{
	switch(key){
		case(KEY_RETURN):case(KEY_TAMA):
			GameExit();
		return FALSE;

		default:
		return TRUE;
	}
}

static bool ExitFnNo(INPUT_BITS key)
{
	switch(key){
		case(KEY_RETURN):case(KEY_TAMA):
			GameRestart();
		return FALSE;

		default:
		return TRUE;
	}
}

static bool ContinueFnYes(INPUT_BITS key)
{
	switch(key){
		case(KEY_RETURN):case(KEY_TAMA):
			GameContinue();
		return FALSE;

		default:
		return TRUE;
	}
}

static bool ContinueFnNo(INPUT_BITS key)
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

static bool InpFnKey(
	INPUT_PAD_BUTTON& config_pad, INPUT_BITS pad_config_key, INPUT_BITS key
)
{
	key &= (~Pad_Data);
	const auto temp = Key_PadSingle();
	if(temp) {
		config_pad = temp.value();
		SetIKeyItem();
	}

	return ((key != KEY_RETURN) && (key != KEY_TAMA));
}

static bool InpFnKeyTama(INPUT_BITS key)
{
	return InpFnKey(ConfigDat.PadTama.v, KEY_TAMA, key);
}

static bool InpFnKeyBomb(INPUT_BITS key)
{
	return InpFnKey(ConfigDat.PadBomb.v, KEY_BOMB, key);
}

static bool InpFnKeyShift(INPUT_BITS key)
{
	return InpFnKey(ConfigDat.PadShift.v, KEY_SHIFT, key);
}

static bool InpFnKeyCancel(INPUT_BITS key)
{
	return InpFnKey(ConfigDat.PadCancel.v, KEY_ESC, key);
}


static bool CfgRepStgSelect(INPUT_BITS key)
{
	return RingFN(SetCfgRepItem, ConfigDat.StageSelect.v, key, 1, STAGE_MAX);
}


static bool CfgRepSave(INPUT_BITS key)
{
	return OptionFN(key, SetCfgRepItem, [] {
		ConfigDat.StageSelect.v = ((ConfigDat.StageSelect.v) ? 0 : 1);
	});
}


static void SetCfgRepItem(void)
{
	if(0 == ConfigDat.StageSelect.v) {
		sprintf(CfgRepTitle[0], "ReplaySave  %s", CHOICE_OFF_ON[false]);
		strcpy(CfgRepTitle[1], "StageSelect [無 効]");
	}
	else{
		sprintf(CfgRepTitle[0], "ReplaySave  %s", CHOICE_OFF_ON[true]);
		sprintf(CfgRepTitle[1], "StageSelect [  %d  ]", ConfigDat.StageSelect.v);
	}
}


static void SetDifItem(void)
{
	const char *DifItem[4] = {" Easy  "," Normal"," Hard  ","Lunatic"};
/*
	{DifTitle[4],	"[DebugMode] 画面に情報を表示するか",	DifFnMsgDisplay,0,0},
	{DifTitle[5],	"[DebugMode] ステージセレクト",	DifFnStgSelect,0,0},
	{DifTitle[6],	"[DebugMode] 当たり判定",	DifFnHit,0,0},
*/
	sprintf(DifTitle[0], "PlayerStock [ %d ]", (ConfigDat.PlayerStock.v + 1));	// +1 に注意
	sprintf(DifTitle[1], "BombStock   [ %d ]", ConfigDat.BombStock.v);
	sprintf(DifTitle[2], "Difficulty[%s]", DifItem[ConfigDat.GameLevel.v]);

#ifdef PBG_DEBUG
	sprintf(DifTitle[4], "DebugOut  %s", CHOICE_OFF_ON[DebugDat.MsgDisplay]);
	sprintf(DifTitle[5], "StgSelect [  %d  ]", DebugDat.StgSelect);
	sprintf(DifTitle[6], "Hit       %s", CHOICE_OFF_ON[DebugDat.Hit]);
	sprintf(DifTitle[7], "DemoSave  %s", CHOICE_OFF_ON[DebugDat.DemoSave]);
#endif
}

static void SetGrpItem(void)
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
}

static void SetSndItem(void)
{
	static int now;
	char	buf[1000];
	static BYTE time = 0;

	const auto sound_active = (ConfigDat.SoundFlags.v & SNDF_SE_ENABLE);
	const auto bgm_active = BGM_Enabled();

	sprintf(SndTitle[0], "Sound  [%s]", CHOICE_USE[!sound_active]);
	sprintf(SndTitle[1], "BGM    [%s]", CHOICE_USE[!bgm_active]);

	const auto maybe_dev = MidBackend_DeviceName();
	if(bgm_active && maybe_dev) {
		time+=16;
		const auto dev = maybe_dev.value();
		if(dev.size() > 18) {
			sprintf(buf, "     %s     %s", dev.data(), dev.data());
			if(time == 0) {
				now = ((now + 1) % (dev.size() + 5));
			}
		}
		else{
			now = 0;
			strcpy(buf, dev.data());
		}
		sprintf(SndTitle[2],">%.18s",buf+now);
		SndItem[2].State = WINDOW_INFO::STATE::REGULAR;
	} else {
		sprintf(SndTitle[2],"        ^^^^^^^^^^");
		SndItem[2].State = WINDOW_INFO::STATE::DISABLED;
	}
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
	constexpr LABELS<4> labels = {{ "Shot", "Bomb", "SpeedDown", "ESC" }};
	auto set = [](char* buf, Narrow::string_view label, INPUT_PAD_BUTTON v) {
		if(v > 0) {
			sprintf(buf, "%-*s[Button%2d]", labels.w, label.data(), v);
		} else {
			sprintf(buf, "%-*s[--------]", labels.w, label.data());
		}
	};
	set(IKeyTitle[0], labels.str[0], ConfigDat.PadTama.v);
	set(IKeyTitle[1], labels.str[1], ConfigDat.PadBomb.v);
	set(IKeyTitle[2], labels.str[2], ConfigDat.PadShift.v);
	set(IKeyTitle[3], labels.str[3], ConfigDat.PadCancel.v);
}
