/*                                                                           */
/*   WINDOWCTRL.cpp   ウィンドウの定義＆管理                                 */
/*                                                                           */
/*                                                                           */

#include "WindowCtrl.h"
#include "WindowSys.h"
#include "CONFIG.H"
#include "ENTRY.H"
#include "FONTUTY.H"
#include "GAMEMAIN.H"
#include "LEVEL.H"
#include "LOADER.H"
#include "platform/input.h"
#include "platform/midi_backend.h"
#include "platform/input.h"
#include "platform/urlopen.h"
#include "game/bgm.h"
#include "game/midi.h"
#include "game/snd.h"
#include "game/string_format.h"



static void DifFnPlayerStock(int_fast8_t delta);
static void DifFnBombStock(int_fast8_t delta);
static void DifFnDifficulty(int_fast8_t delta);

#ifdef PBG_DEBUG
static void DifFnMsgDisplay(int_fast8_t delta);
static void DifFnStgSelect(int_fast8_t delta);
static void DifFnHit(int_fast8_t delta);
static void DifFnDemo(int_fast8_t delta);
#endif

static void GrpFnChgDevice(int_fast8_t delta);
static void GrpFnSkip(int_fast8_t delta);
static void GrpFnBpp(int_fast8_t delta);
static void GrpFnWinLocate(int_fast8_t delta);

static void SndFnSE(int_fast8_t delta);
static void SndFnBGM(int_fast8_t delta);
static void SndFnSEVol(int_fast8_t delta);
static void SndFnBGMVol(int_fast8_t delta);
static void SndFnBGMGain(int_fast8_t delta);
static void SndFnBGMPack(int_fast8_t delta);
static void SndFnMIDIDev(int_fast8_t delta);

static void InpFnMsgSkip(int_fast8_t delta);
static void InpFnZSpeedDown(int_fast8_t delta);

static bool InpFnKeyTama(INPUT_BITS key);
static bool InpFnKeyBomb(INPUT_BITS key);
static bool InpFnKeyShift(INPUT_BITS key);
static bool InpFnKeyCancel(INPUT_BITS key);

static void CfgRepStgSelect(int_fast8_t delta);
static void CfgRepSave(int_fast8_t delta);

static bool MainFnGameStart(INPUT_BITS key);
static bool MainFnExStart(INPUT_BITS key);

static bool MusicFn(INPUT_BITS key);

static bool ExitFnYes(INPUT_BITS key);
static bool ExitFnNo(INPUT_BITS key);

static bool ContinueFnYes(INPUT_BITS key);
static bool ContinueFnNo(INPUT_BITS key);

static bool ScoreFn(INPUT_BITS key);

static void SetDifItem(bool tick = true);
static void SetGrpItem(bool tick = true);
static void SetSndItem(bool tick = true);
static void SetInpItem(bool tick = true);
static void SetIKeyItem(bool tick = true);
static void SetCfgRepItem(bool tick = true);

static bool RFnStg1(INPUT_BITS key);
static bool RFnStg2(INPUT_BITS key);
static bool RFnStg3(INPUT_BITS key);
static bool RFnStg4(INPUT_BITS key);
static bool RFnStg5(INPUT_BITS key);
static bool RFnStg6(INPUT_BITS key);
static bool RFnStgEx(INPUT_BITS key);

static constexpr void RingStep(
	uint8_t& var, int_fast8_t delta, uint8_t min, uint8_t max
)
{
	var = ((delta < 0)
		? ((var <= min) ? max : (var - 1))
		: ((var == max) ? min : (var + 1))
	);
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
static constexpr const char* CHOICE_OFF_ON_NARROW[2]  = { "[  ]", "[●]" };
static constexpr const char* CHOICE_USE[2] = { " 使用する ", "使用しない" };

namespace BGMPack {
	void Open(void);

	constexpr const char* SOUNDTRACK_URL = (
		"https://github.com/nmlgc/BGMPacks/releases/tag/P0269"
	);
	constexpr const char* HELP_DOWNLOAD = (
		"収録のサントラをダウンロードします"
	);
	constexpr const char* HELP_SET = "BGMパックのメニューを開きます";

	constexpr Narrow::string_view TITLE = " BGM pack";
	constexpr Narrow::string_view TITLE_FMT = " BGM pack (%zu/%zu)";
	constexpr Narrow::string_view TITLE_NONE = "<使用しない>";
	constexpr Narrow::string_view TITLE_DOWNLOAD = "<Download>";
	constexpr auto HELP_NONE = "デフォルトのMIDIサントラに戻ります";

	char Title[TITLE_FMT.size() + (STRING_NUM_CAP<size_t> * 2)];
	WINDOW_LABEL TitleItem = { Title };

	std::vector<std::u8string> Packs;
	size_t SelAtOpen = 0;

	// Scroll functions
	// ----------------

	static size_t ListSize(void) {
		return (1 + Packs.size() + 1);
	}
	static void Generate(WINDOW_CHOICE& ret, size_t generated, size_t selected);
	static bool Handle(INPUT_BITS key, size_t selected);
	// ----------------
}

constexpr auto HELP_SUBMENU_EXIT = "一つ前のメニューにもどります";

constexpr WINDOW_CHOICE HRuleItemForArray = { "-------------------" };
constexpr WINDOW_CHOICE SubmenuExitItemForArray = {
	"Exit", HELP_SUBMENU_EXIT, CWinExitFn
};
WINDOW_CHOICE SubmenuExitItem = SubmenuExitItemForArray;
WINDOW_CHOICE HRuleItem = HRuleItemForArray;

char	DifTitle[9][20];
WINDOW_CHOICE DifItem[] = {
	{ DifTitle[0],	"残り人数?を設定します",	DifFnPlayerStock },
	{ DifTitle[1],	"ボムの数を設定します",	DifFnBombStock },
	{ DifTitle[2],	"難易度を設定します",	DifFnDifficulty },
#ifdef PBG_DEBUG
	HRuleItemForArray,
	{ DifTitle[4],	"[DebugMode] 画面に情報を表示するか",	DifFnMsgDisplay },
	{ DifTitle[5],	"[DebugMode] ステージセレクト",	DifFnStgSelect },
	{ DifTitle[6],	"[DebugMode] 当たり判定",	DifFnHit },
	{ DifTitle[7],	"[DebugMode] デモプレイセーブ",	DifFnDemo },
#endif
	SubmenuExitItemForArray,
};
WINDOW_MENU DifMenu = { std::span(DifItem), SetDifItem };

char	GrpTitle[5][50];
WINDOW_CHOICE GrpItem[] = {
	// { GrpTitle[0],	"ビデオカードの選択",	GrpFnChgDevice },
	{ GrpTitle[1],	"描画スキップの設定です",	GrpFnSkip },
	{ GrpTitle[2],	"使用する色数を指定します",	GrpFnBpp },
	{ GrpTitle[3],	"ウィンドウの表示位置を決めます",	GrpFnWinLocate },
	SubmenuExitItem,
};
WINDOW_MENU GrpMenu = { std::span(GrpItem), SetGrpItem };

constexpr auto VOLUME_FLAGS = WINDOW_FLAGS::FAST_REPEAT;

static char SndTitleSE[26];
static char SndTitleBGM[26];
static char SndTitleSEVol[26];
static char SndTitleBGMVol[26];
static char SndTitleBGMGain[26];
static char SndTitleBGMPack[26];
static char SndTitleMIDIPort[26];
WINDOW_CHOICE SndItem[] = {
	{ SndTitleSE,	"SEを鳴らすかどうかの設定",	SndFnSE },
	{ SndTitleBGM,	"BGMを鳴らすかどうかの設定",	SndFnBGM },
	{ SndTitleSEVol, "効果音の音量", SndFnSEVol, VOLUME_FLAGS },
	{ SndTitleBGMVol, "音楽の音量", SndFnBGMVol, VOLUME_FLAGS },
	{ SndTitleBGMGain,	"毎に曲から音量の違うことが外します",	SndFnBGMGain },
	{ SndTitleBGMPack,	BGMPack::HELP_DOWNLOAD,	SndFnBGMPack },
	{ SndTitleMIDIPort,	"MIDI Port (保存はされません)",	SndFnMIDIDev },
	SubmenuExitItemForArray,
};
WINDOW_MENU SndMenu = { std::span(SndItem), SetSndItem };
static auto& SndItemSE = SndItem[0];
static auto& SndItemBGM = SndItem[1];
static auto& SndItemSEVol = SndItem[2];
static auto& SndItemBGMVol = SndItem[3];
static auto& SndItemBGMGain = SndItem[4];
static auto& SndItemBGMPack = SndItem[5];
static auto& SndItemMIDIPort = SndItem[6];

char IKeyTitle[4][20];
char InpHelp[] = "パッド上のボタンを押すと変更";
WINDOW_CHOICE InpKey[] = {
	{ IKeyTitle[0],	InpHelp,	InpFnKeyTama },
	{ IKeyTitle[1],	InpHelp,	InpFnKeyBomb },
	{ IKeyTitle[2],	InpHelp,	InpFnKeyShift },
	{ IKeyTitle[3],	InpHelp,	InpFnKeyCancel },
	SubmenuExitItemForArray,
};
WINDOW_MENU InpKeyMenu = { std::span(InpKey), SetIKeyItem };

char	InpTitle[23];
char	InpTitle2[23];
WINDOW_CHOICE InpItem[] = {
	{ InpTitle,	"弾キーのメッセージスキップ設定",	InpFnMsgSkip },
	{ InpTitle2,	"弾キーの押しっぱなしで低速移動",	InpFnZSpeedDown },
	{ "Joy Pad",	"パッドの設定をします",	InpKeyMenu },
	SubmenuExitItemForArray,
};
WINDOW_MENU InpMenu = { std::span(InpItem), SetInpItem };

char CfgRepTitle[2][23];

WINDOW_CHOICE CfgRep[] = {
	{ CfgRepTitle[0],	"リプレイ用データの保存",	CfgRepSave },
	{ CfgRepTitle[1],	"ステージセレクト",	CfgRepStgSelect },
	SubmenuExitItemForArray,
};
WINDOW_MENU CfgRepMenu = { std::span(CfgRep), SetCfgRepItem };

WINDOW_CHOICE CfgItem[] = {
	{ " Difficulty",	"難易度に関する設定",	DifMenu },
	{ " Graphic",	"グラフィックに関する設定",	GrpMenu },
	{ " Sound / Music",	"ＳＥ／ＢＧＭに関する設定",	SndMenu },
	{ " Input",	"入力デバイスに関する設定",	InpMenu },
	{ " Replay",	"リプレイに関する設定",	CfgRepMenu },
	{ " Exit",	HELP_SUBMENU_EXIT,	CWinExitFn },
};
WINDOW_MENU CfgMenu = { std::span(CfgItem) };

WINDOW_CHOICE RepItem[] = {
	{ " Stage 1 デモ再生",	"ステージ１のリプレイ",	RFnStg1 },
	{ " Stage 2 デモ再生",	"ステージ２のリプレイ",	RFnStg2 },
	{ " Stage 3 デモ再生",	"ステージ３のリプレイ",	RFnStg3 },
	{ " Stage 4 デモ再生",	"ステージ４のリプレイ",	RFnStg4 },
	{ " Stage 5 デモ再生",	"ステージ５のリプレイ",	RFnStg5 },
	{ " Stage 6 デモ再生",	"ステージ６のリプレイ",	RFnStg6 },
	{ " ExStage デモ再生",	"エキストラステージのリプレイ",	RFnStgEx },
	{ " Exit",	HELP_SUBMENU_EXIT,	CWinExitFn },
};
WINDOW_MENU RepMenu = { std::span(RepItem) };

WINDOW_LABEL MainTitle = { "     Main Menu" };
WINDOW_CHOICE MainItem[] = {
	// { "   Game  Start",	"ゲームを開始します(使用不可)" },
	{ "   Game  Start",	"ゲームを開始します",	MainFnGameStart },
	{ "   Extra Start",	"ゲームを開始します(Extra)",	MainFnExStart },
	{ "   Replay",	"リプレイを開始します",	RepMenu },
	{ "   Config",	"各種設定を変更します",	CfgMenu },
	{ "   Score",	"スコアの表示をします",	ScoreFn },
	{ "   Music",	"音楽室に入ります",	MusicFn },
	{ "   Exit",	"ゲームを終了します",	CWinExitFn }
};
WINDOW_MENU MainMenu = { std::span(MainItem), [](bool) {}, &MainTitle };

WINDOW_LABEL ExitTitle = { "    終了するの？" };
WINDOW_CHOICE ExitYesNoItem[] = {
	{ "   お っ け ～ ",	"",	ExitFnYes },
	{ "   だ め だ め",	"",	ExitFnNo }
};
WINDOW_MENU ExitMenu = { std::span(ExitYesNoItem), [](bool) {}, &ExitTitle };

WINDOW_LABEL ContinueTitle = { " Ｃｏｎｔｉｎｕｅ？" };
WINDOW_CHOICE ContinueYesNoItem[] = {
	{ "   お っ け ～",	"",	ContinueFnYes },
	{ "   や だ や だ",	"",	ContinueFnNo }
};
WINDOW_MENU ContinueMenu = {
	std::span(ContinueYesNoItem), [](bool) {}, &ContinueTitle
};

WINDOW_MENU_SCROLL<
	BGMPack::TitleItem, BGMPack::ListSize, BGMPack::Generate, BGMPack::Handle
> BGMPackMenu;


///// [グローバル変数(公開)] /////
WINDOW_SYSTEM MainWindow = { MainMenu };
WINDOW_SYSTEM ExitWindow = { ExitMenu };
WINDOW_SYSTEM ContinueWindow = { ContinueMenu };
WINDOW_SYSTEM BGMPackWindow = { BGMPackMenu.Menu };



// メインメニューの初期化 //
void InitMainWindow(void)
{
	MainWindow.Init(140);

	//	{ "   Config",	"各種設定を変更します",	CfgItem },

	// エキストラステージが選択できる場合には発生！ //
	if(ConfigDat.ExtraStgFlags.v) {
		CfgMenu.NumItems = 6;
		CfgMenu.ItemPtr[4] = &CfgItem[4];
		CfgMenu.ItemPtr[5] = &CfgItem[5];
	}
	else{
		CfgMenu.NumItems = 5;
		CfgMenu.ItemPtr[4] = &CfgItem[5];
	}
}


void InitExitWindow(void)
{
	ExitWindow.Init(140);
}

void InitContinueWindow(void)
{
	ContinueWindow.Init(140);
}

static void DifFnPlayerStock(int_fast8_t delta)
{
	RingStep(ConfigDat.PlayerStock.v, delta, 0, STOCK_PLAYER_MAX);
}

static void DifFnBombStock(int_fast8_t delta)
{
	RingStep(ConfigDat.BombStock.v, delta, 0, STOCK_BOMB_MAX);
}

static void DifFnDifficulty(int_fast8_t delta)
{
	RingStep(ConfigDat.GameLevel.v, delta, GAME_EASY, GAME_LUNATIC);
}

#ifdef PBG_DEBUG
static void DifFnMsgDisplay(int_fast8_t)
{
	DebugDat.MsgDisplay = !DebugDat.MsgDisplay;
}

static void DifFnStgSelect(int_fast8_t delta)
{
	RingStep(DebugDat.StgSelect, delta, 1, STAGE_MAX);
}

static void DifFnHit(int_fast8_t)
{
	DebugDat.Hit = !DebugDat.Hit;
}

static void DifFnDemo(int_fast8_t)
{
	DebugDat.DemoSave = !DebugDat.DemoSave;
}

#endif // PBG_DEBUG

static void GrpFnChgDevice(int_fast8_t delta)
{
	XGrpTry([&](auto& params) {
		// 一つしかデバイスが存在しないときは変更できない //
		const auto device_count = GrpBackend_DeviceCount();
		if(device_count <= 1) {
			return;
		}

		// 次のデバイスへ //
		params.device_id = (
			(ConfigDat.DeviceID.v + device_count + delta) % device_count
		);
	});
}

static void GrpFnSkip(int_fast8_t delta)
{
	RingStep(ConfigDat.FPSDivisor.v, delta, 0, FPS_DIVISOR_MAX);
	Grp_FPSDivisor = ConfigDat.FPSDivisor.v;
}

static void GrpFnBpp(int_fast8_t delta)
{
	XGrpTry([delta](auto& params) {
		params.bitdepth = ConfigDat.BitDepth.v.cycle(delta < 0);
	});
}

static void GrpFnWinLocate(int_fast8_t delta)
{
	static constexpr uint8_t flags[3] = {
		0, GRPF_WINDOW_UPPER, GRPF_MSG_DISABLE
	};
	int		i;

	for(i=0; i<3; i++){
		if(ConfigDat.GraphFlags.v == flags[i]) break;
	}
	if(i >= 3) i=0;

	if(delta < 0) {
		ConfigDat.GraphFlags.v = flags[(i + 2) % 3];
	} else {
		ConfigDat.GraphFlags.v = flags[(i + 1) % 3];
	}
}

static void SndFnSE(int_fast8_t)
{
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
}

static void SndFnBGM(int_fast8_t)
{
	if(BGM_Enabled()) {
		BGM_Cleanup();
	} else {
		// 成功した場合にだけ有効にする //
		if(BGM_Init()) {
			BGM_Switch(0);
		}
	}
}

static void SndFnSEVol(int_fast8_t delta)
{
	ConfigDat.SEVolume.v = std::clamp(
		(ConfigDat.SEVolume.v + delta), 0, int{ VOLUME_MAX }
	);
	Snd_UpdateVolumes();
}

static void SndFnBGMVol(int_fast8_t delta)
{
	ConfigDat.BGMVolume.v = std::clamp(
		(ConfigDat.BGMVolume.v + delta), 0, int{ VOLUME_MAX }
	);
	BGM_UpdateVolume();
}

static void SndFnBGMPack(int_fast8_t)
{
	if(!BGM_PacksAvailable()) {
		URLOpen(BGMPack::SOUNDTRACK_URL);
	} else {
		BGMPack::Open();
	}
}

static void SndFnBGMGain(int_fast8_t)
{
	BGM_SetGainApply(!BGM_GainApply());
}

namespace BGMPack {
	size_t SelNone(void) {
		return 0;
	}
	size_t SelDownload(void) {
		return (ListSize() - 1);
	}

	static void Open(void)
	{
		PIXEL_COORD w = CWinItemExtent(TITLE_FMT).w;
		w = (std::max)(w, CWinTextExtent(TITLE_DOWNLOAD).w);
		w = (std::max)(w, CWinTextExtent(TITLE_NONE).w);
		Packs.clear();
		Packs.reserve(BGM_PackCount());
		BGM_PackForeach([](const auto&& str) {
			Packs.emplace_back(std::move(str));
		});
		std::ranges::sort(Packs);
		SelAtOpen = SelNone();
		for(size_t i = 1; const auto& pack : Packs) {
			if(pack == ConfigDat.BGMPack.v) {
				SelAtOpen = i;
			}
			w = (std::max)(w, CWinItemExtent(pack).w);
			i++;
		}
		w = (std::min)(w, GRP_RES.w);

		BGMPackMenu.Init(BGMPackWindow, SelAtOpen, &MainWindow);
		BGMPackWindow.Init(w);
		BGMPackWindow.OpenCentered(w, BGMPackWindow.Select[0]);
	}

	static void Generate(WINDOW_CHOICE& ret, size_t generated, size_t selected)
	{
		const auto sel_none = SelNone();
		const auto sel_download = SelDownload();
		if(generated == sel_none) {
			ret.Title = TITLE_NONE.data();
			ret.Help = HELP_NONE;
		} else if(generated == sel_download) {
			ret.Title = TITLE_DOWNLOAD.data();
			ret.Help = HELP_DOWNLOAD;
		} else {
			ret.Title = Packs[generated - 1];
			ret.Help = "";
		}
		ret.State = ((generated == SelAtOpen)
			? WINDOW_STATE::HIGHLIGHT
			: WINDOW_STATE::REGULAR
		);
		if(generated == selected) {
			if((generated == sel_none) || (generated == sel_download)) {
				std::ranges::copy(TITLE, Title).out[0] = '\0';
			} else {
				sprintf(Title, TITLE_FMT.data(), generated, Packs.size());
			}
		}
	}

	static bool Handle(INPUT_BITS key, size_t selected)
	{
		if(CWinOKKey(key)) {
			if(selected == SelDownload()) {
				URLOpen(BGMPack::SOUNDTRACK_URL);
			} else {
				if(selected == SelNone()) {
					ConfigDat.BGMPack.v.clear();
				} else {
					ConfigDat.BGMPack.v = Packs[selected - 1];
				}
				SetSndItem();
				BGM_PackSet(ConfigDat.BGMPack.v);
			}
			return false;
		}
		return true;
	}
}

static void SndFnMIDIDev(int_fast8_t delta)
{
	if(BGM_Enabled()) {
		BGM_ChangeMIDIDevice(delta);
	}
}

static void InpFnMsgSkip(int_fast8_t)
{
	if(ConfigDat.InputFlags.v & INPF_Z_MSKIP_ENABLE) {
		ConfigDat.InputFlags.v &= (~INPF_Z_MSKIP_ENABLE);
	} else {
		ConfigDat.InputFlags.v |= INPF_Z_MSKIP_ENABLE;
	}
}

static void InpFnZSpeedDown(int_fast8_t)
{
	if(ConfigDat.InputFlags.v & INPF_Z_SPDDOWN_ENABLE) {
		ConfigDat.InputFlags.v &= (~INPF_Z_SPDDOWN_ENABLE);
	} else {
		ConfigDat.InputFlags.v |= INPF_Z_SPDDOWN_ENABLE;
	}
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
	if(CWinOKKey(key)) {
		//EndingInit();
		WeaponSelectInit(false);
	}
	return true;
}

static bool MainFnExStart(INPUT_BITS key)
{
	if(CWinOKKey(key)) {
		if(ConfigDat.ExtraStgFlags.v) {
			WeaponSelectInit(true);
		}
	}
	return true;
}

static bool ScoreFn(INPUT_BITS key)
{
	if(CWinOKKey(key)) {
		ScoreNameInit();
	}
	return true;
}

static bool MusicFn(INPUT_BITS key)
{
	if(CWinOKKey(key)) {
		if(BGM_Enabled()) {
			MusicRoomInit();
		}
	}
	return true;
}

static bool ExitFnYes(INPUT_BITS key)
{
	if(CWinOKKey(key)) {
		GameExit();
		return false;
	}
	return true;
}

static bool ExitFnNo(INPUT_BITS key)
{
	if(CWinOKKey(key)) {
		GameRestart();
		return false;
	}
	return true;
}

static bool ContinueFnYes(INPUT_BITS key)
{
	if(CWinOKKey(key)) {
		GameContinue();
		return false;
	}
	return true;
}

static bool ContinueFnNo(INPUT_BITS key)
{
	if(CWinOKKey(key)) {
		NameRegistInit(true);
		// GameExit();
		return false;
	}
	return true;
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


static void CfgRepStgSelect(int_fast8_t delta)
{
	RingStep(ConfigDat.StageSelect.v, delta, 1, STAGE_MAX);
}


static void CfgRepSave(int_fast8_t)
{
	ConfigDat.StageSelect.v = ((ConfigDat.StageSelect.v) ? 0 : 1);
}


static void SetCfgRepItem(bool)
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


static void SetDifItem(bool)
{
	const char *const dif[4] = {" Easy  "," Normal"," Hard  ","Lunatic" };
/*
	{DifTitle[4],	"[DebugMode] 画面に情報を表示するか",	DifFnMsgDisplay,0,0},
	{DifTitle[5],	"[DebugMode] ステージセレクト",	DifFnStgSelect,0,0},
	{DifTitle[6],	"[DebugMode] 当たり判定",	DifFnHit,0,0},
*/
	sprintf(DifTitle[0], "PlayerStock [ %d ]", (ConfigDat.PlayerStock.v + 1));	// +1 に注意
	sprintf(DifTitle[1], "BombStock   [ %d ]", ConfigDat.BombStock.v);
	sprintf(DifTitle[2], "Difficulty[%s]", dif[ConfigDat.GameLevel.v]);

#ifdef PBG_DEBUG
	sprintf(DifTitle[4], "DebugOut  %s", CHOICE_OFF_ON[DebugDat.MsgDisplay]);
	sprintf(DifTitle[5], "StgSelect [  %d  ]", DebugDat.StgSelect);
	sprintf(DifTitle[6], "Hit       %s", CHOICE_OFF_ON[DebugDat.Hit]);
	sprintf(DifTitle[7], "DemoSave  %s", CHOICE_OFF_ON[DebugDat.DemoSave]);
#endif
}

static void SetGrpItem(bool)
{
	const char *const UorD[3] = { "上のほう", "下のほう", "描画せず" };
	const char *const DMode[4] = { "おまけ", "60Fps", "30Fps", "20Fps" };
	int		i;

#define SetFlagsMacro(src,flag)		((flag) ? src[0] : src[1])

	const auto dev = GrpBackend_DeviceName(ConfigDat.DeviceID.v);
	sprintf(GrpTitle[0], "Device   [%.7s]", dev.data());
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

static void SetSndItem(bool tick)
{
	static int now;
	char	buf[1000];
	static BYTE time = 0;

	const auto sound_active = (ConfigDat.SoundFlags.v & SNDF_SE_ENABLE);
	const auto bgm_active = BGM_Enabled();

	// Additionally purge the cache at the initialization of the main menu,
	// and when moving between options.
	if(
		(MainWindow.State == CWIN_DEAD) ||
		(MainWindow.OldKey == KEY_UP) ||
		(MainWindow.OldKey == KEY_DOWN)
	) {
		BGM_PacksAvailable(true);
	}

	const auto norm_choice = CHOICE_OFF_ON_NARROW[BGM_GainApply()];
	SndItemSEVol.SetActive(sound_active);
	SndItemBGMVol.SetActive(bgm_active);
	SndItemBGMGain.SetActive(bgm_active && BGM_HasGainFactor());

	sprintf(SndTitleSE,      "Sound  [%s]", CHOICE_USE[!sound_active]);
	sprintf(SndTitleBGM,     "BGM    [%s]", CHOICE_USE[!bgm_active]);
	sprintf(SndTitleSEVol,   "SoundVolume [ %3d ]", ConfigDat.SEVolume.v);
	sprintf(SndTitleBGMVol,  "BGMVolume   [ %3d ]", ConfigDat.BGMVolume.v);
	sprintf(SndTitleBGMGain, "BGMVolNormalize%s", norm_choice);
	if(!BGM_PacksAvailable()) {
		sprintf(SndTitleBGMPack, "BGMPack[ Download ]");
		SndItemBGMPack.Help = BGMPack::HELP_DOWNLOAD;
	} else if(ConfigDat.BGMPack.v.empty()) {
		sprintf(SndTitleBGMPack, "BGMPack[%s]", CHOICE_USE[1]);
		SndItemBGMPack.Help = BGMPack::HELP_SET;
	} else {
		sprintf(SndTitleBGMPack, "BGMPack[   ....   ]");
		SndItemBGMPack.Help = BGMPack::HELP_SET;
	}

	const auto maybe_dev = MidBackend_DeviceName();
	if(bgm_active && maybe_dev) {
		if(tick) {
			time += 16;
		}
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
		sprintf(SndTitleMIDIPort, ">%.18s", (buf + now));
		SndItemMIDIPort.State = WINDOW_STATE::REGULAR;
	} else {
		strcpy(SndTitleMIDIPort, ">");
		SndItemMIDIPort.State = WINDOW_STATE::DISABLED;
	}
}

static void SetInpItem(bool)
{
	int		temp;

	temp = ((ConfigDat.InputFlags.v & INPF_Z_MSKIP_ENABLE) ? 1 : 0);
	sprintf(InpTitle,"Z-MessageSkip[%s]",temp ? "ＯＫ" : "禁止");

	temp = ((ConfigDat.InputFlags.v & INPF_Z_SPDDOWN_ENABLE) ? 1 : 0);
	sprintf(InpTitle2,"Z-SpeedDown  [%s]",temp ? "ＯＫ" : "禁止");
}

static void SetIKeyItem(bool)
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
