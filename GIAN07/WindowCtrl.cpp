/*                                                                           */
/*   WINDOWCTRL.cpp   ウィンドウの定義＆管理                                 */
/*                                                                           */
/*                                                                           */

#include "WindowCtrl.h"
#include "WindowSys.h"
#include "CONFIG.H"
#include "ENTRY.H"
#include "GAMEMAIN.H"
#include "LEVEL.H"
#include "LOADER.H"
#include "MUSIC.H"
#include "platform/input.h"
#include "platform/midi_backend.h"
#include "platform/input.h"
#include "platform/urlopen.h"
#include "game/bgm.h"
#include "game/midi.h"
#include "game/snd.h"
#include "game/string_format.h"



static bool ExitFnYes(INPUT_BITS key);
static bool ExitFnNo(INPUT_BITS key);

static bool ContinueFnYes(INPUT_BITS key);
static bool ContinueFnNo(INPUT_BITS key);

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
	const int w;

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
		"https://github.com/nmlgc/BGMPacks/releases/tag/2024-10-05"
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
constexpr auto HELP_API_DEFAULT  = "Let the backend choose a graphics API";
constexpr auto HELP_API_SPECIFIC = "Select to override default API selection";

constexpr WINDOW_CHOICE HRuleItemForArray = { "-------------------" };
constexpr WINDOW_CHOICE SubmenuExitItemForArray = {
	"Exit", HELP_SUBMENU_EXIT, CWinExitFn
};
constinit WINDOW_CHOICE SubmenuExitItem = SubmenuExitItemForArray;
constinit WINDOW_CHOICE HRuleItem = HRuleItemForArray;

namespace Main {
namespace Cfg {
namespace Dif {
static void FnPlayerStock(int_fast8_t delta);
static void FnBombStock(int_fast8_t delta);
static void FnDifficulty(int_fast8_t delta);
#ifdef PBG_DEBUG
static void FnMsgDisplay(int_fast8_t delta);
static void FnStgSelect(int_fast8_t delta);
static void FnHit(int_fast8_t delta);
static void FnDemo(int_fast8_t delta);
#endif
static void SetItem(bool tick = true);

char Title[7][20];
WINDOW_CHOICE Item[] = {
	{ Title[0], "残り人数?を設定します", FnPlayerStock },
	{ Title[1], "ボムの数を設定します", FnBombStock },
	{ Title[2], "難易度を設定します", FnDifficulty },
#ifdef PBG_DEBUG
	HRuleItemForArray,
	{ Title[3], "[DebugMode] 画面に情報を表示するか", FnMsgDisplay },
	{ Title[4], "[DebugMode] ステージセレクト", FnStgSelect },
	{ Title[5], "[DebugMode] 当たり判定", FnHit },
	{ Title[6], "[DebugMode] デモプレイセーブ", FnDemo },
#endif
	SubmenuExitItemForArray,
};
WINDOW_MENU Menu = { std::span(Item), SetItem };
} // namespace Dif

namespace Grp {
namespace Screenshot {
static void SetItem(bool tick = true);

WINDOW_CHOICE Item[] = {
	SubmenuExitItemForArray,
};
WINDOW_MENU Menu = { std::span(Item), SetItem };
} // namespace Screenshot

namespace API {
static void FnDef(int_fast8_t delta);
static void FnOverride(int_fast8_t delta);
static void SetItem(bool tick = true);

#ifdef SUPPORT_GRP_API
char TitleDef[26];
WINDOW_CHOICE ItemDef = { TitleDef, HELP_API_DEFAULT, FnDef };
constinit WINDOW_CHOICE Item[8];
WINDOW_MENU Menu = { std::span<WINDOW_CHOICE, 0>(), SetItem };
#endif
}

static void FnChgDevice(int_fast8_t delta);
static void FnDisp(int_fast8_t delta);
static void FnFSMode(int_fast8_t delta);
static void FnScale(int_fast8_t delta);
static void FnScMode(int_fast8_t delta);
static void FnSkip(int_fast8_t delta);
static void FnBpp(int_fast8_t delta);
static void FnWinLocate(int_fast8_t delta);
static void SetItem(bool tick = true);

static char TitleDevice[50];
#ifdef SUPPORT_GRP_WINDOWED
static char TitleDisp[50];
static char TitleFSMode[50];
#endif
#ifdef SUPPORT_GRP_SCALING
static char TitleScale[50];
static char TitleScMode[50];
#endif
static char TitleSkip[50];
#ifdef SUPPORT_GRP_BITDEPTH
static char TitleBpp[50];
#endif
static char TitleMsg[50];

#ifdef SUPPORT_GRP_WINDOWED
static char HelpFSMode[50];
#endif
#ifdef SUPPORT_GRP_SCALING
static char HelpScale[50];
static char HelpScMode[50];
#endif

// WINDOW_CHOICE ItemDevice = {
// 	TitleDevice, "ビデオカードの選択", FnChgDevice
// };
#ifdef SUPPORT_GRP_WINDOWED
WINDOW_CHOICE ItemDisp = {
	TitleDisp, "Switch between window and fullscreen modes", FnDisp
};
WINDOW_CHOICE ItemFSMode = { TitleFSMode, HelpFSMode, FnFSMode };
#endif
#ifdef SUPPORT_GRP_SCALING
WINDOW_CHOICE ItemScale = { TitleScale, HelpScale, FnScale };
WINDOW_CHOICE ItemScMode = { TitleScMode, HelpScMode, FnScMode };
#endif
WINDOW_CHOICE ItemSkip = { TitleSkip, "描画スキップの設定です", FnSkip };
#ifdef SUPPORT_GRP_BITDEPTH
WINDOW_CHOICE ItemBpp = { TitleBpp, "使用する色数を指定します", FnBpp };
#endif
WINDOW_CHOICE ItemMsg = {
	TitleMsg, "ウィンドウの表示位置を決めます", FnWinLocate
};
WINDOW_CHOICE ItemScreenshot = {
	"Screenshots", "Customize the screenshot format", Screenshot::Menu
};
#ifdef SUPPORT_GRP_API
WINDOW_CHOICE ItemAPI = { "API", "Select rendering API", API::Menu };
#endif
WINDOW_CHOICE ItemExit = SubmenuExitItemForArray;
WINDOW_MENU Menu = { SetItem, {
#ifdef SUPPORT_GRP_WINDOWED
	&ItemDisp,
	&ItemFSMode,
#endif
#ifdef SUPPORT_GRP_SCALING
	&ItemScale,
	&ItemScMode,
#endif
	&ItemSkip,
#ifdef SUPPORT_GRP_BITDEPTH
	&ItemBpp,
#endif
	&ItemScreenshot,
#ifdef SUPPORT_GRP_API
	&ItemAPI,
#endif
	&HRuleItem,
	&ItemMsg,
	&ItemExit,
} };
} // namespace Grp

namespace Snd {
namespace Mid {
static void FnDev(int_fast8_t delta);
static void FnFixes(int_fast8_t delta);
static void SetItem(bool tick = true);

static char TitlePort[26];
static char TitleFixes[26];
WINDOW_CHOICE Item[] = {
	{ TitlePort, "MIDI Port (保存はされません)", FnDev },
	{ TitleFixes, "Retain SC-88Pro echo on other Roland synths", FnFixes },
	SubmenuExitItemForArray,
};
#ifdef SUPPORT_MIDI_BACKEND
WINDOW_MENU Menu = { std::span(Item), SetItem };
static auto& ItemPort = Item[0];
#endif
} // namespace Mid

static void FnSE(int_fast8_t delta);
static void FnBGM(int_fast8_t delta);
static void FnSEVol(int_fast8_t delta);
static void FnBGMVol(int_fast8_t delta);
static void FnBGMGain(int_fast8_t delta);
static void FnBGMPack(int_fast8_t delta);
static void SetItem(bool tick = true);

static char TitleSE[26];
static char TitleBGM[26];
static char TitleSEVol[26];
static char TitleBGMVol[26];
static char TitleBGMGain[26];
static char TitleBGMPack[26];
WINDOW_CHOICE Item[] = {
	{ TitleSE, "SEを鳴らすかどうかの設定", FnSE },
	{ TitleBGM, "BGMを鳴らすかどうかの設定", FnBGM },
	{ TitleSEVol, "効果音の音量", FnSEVol, WINDOW_FLAGS::FAST_REPEAT },
	{ TitleBGMVol, "音楽の音量", FnBGMVol, WINDOW_FLAGS::FAST_REPEAT },
	{ TitleBGMGain, "毎に曲から音量の違うことが外します", FnBGMGain },
	{ TitleBGMPack, BGMPack::HELP_DOWNLOAD, FnBGMPack },
#ifdef SUPPORT_MIDI_BACKEND
	{ "MIDI", "Change MIDI playback options", Mid::Menu },
#endif
	SubmenuExitItemForArray,
};
WINDOW_MENU Menu = { std::span(Item), SetItem };
static auto& ItemSE = Item[0];
static auto& ItemBGM = Item[1];
static auto& ItemSEVol = Item[2];
static auto& ItemBGMVol = Item[3];
static auto& ItemBGMGain = Item[4];
static auto& ItemBGMPack = Item[5];
#ifdef SUPPORT_MIDI_BACKEND
static auto& ItemMIDI = Item[6];
#endif
} // namespace Snd

namespace Inp {
namespace Pad {
template <INPUT_PAD_BUTTON& ConfigPad> bool Fn(INPUT_BITS key);
static void SetItem(bool tick = true);

char Title[4][20];
char Help[] = "パッド上のボタンを押すと変更";
WINDOW_CHOICE InpKey[] = {
	{ Title[0], Help, Fn<ConfigDat.PadTama.v> },
	{ Title[1], Help, Fn<ConfigDat.PadBomb.v> },
	{ Title[2], Help, Fn<ConfigDat.PadShift.v> },
	{ Title[3], Help, Fn<ConfigDat.PadCancel.v> },
	SubmenuExitItemForArray,
};
WINDOW_MENU Menu = { std::span(InpKey), SetItem };
} // namespace Pad

static void FnMsgSkip(int_fast8_t delta);
static void FnZSpeedDown(int_fast8_t delta);
static void SetItem(bool tick = true);

char Title[2][23];
WINDOW_CHOICE Item[] = {
	{ Title[0], "弾キーのメッセージスキップ設定", FnMsgSkip },
	{ Title[1], "弾キーの押しっぱなしで低速移動", FnZSpeedDown },
	{ "Joy Pad", "パッドの設定をします", Pad::Menu },
	SubmenuExitItemForArray,
};
WINDOW_MENU Menu = { std::span(Item), SetItem };
} // namespace Inp

namespace Rep {
static void FnStgSelect(int_fast8_t delta);
static void FnSave(int_fast8_t delta);
static void SetItem(bool tick = true);

char Title[2][23];

WINDOW_CHOICE Item[] = {
	{ Title[0], "リプレイ用データの保存", FnSave },
	{ Title[1], "ステージセレクト", FnStgSelect },
	SubmenuExitItemForArray,
};
WINDOW_MENU Menu = { std::span(Item), SetItem };
} // namespace Rep

WINDOW_CHOICE Item[] = {
	{ " Difficulty", "難易度に関する設定", Dif::Menu },
	{ " Graphic", "グラフィックに関する設定", Grp::Menu },
	{ " Sound / Music", "ＳＥ／ＢＧＭに関する設定", Snd::Menu },
	{ " Input", "入力デバイスに関する設定", Inp::Menu },
	{ " Replay", "リプレイに関する設定", Rep::Menu },
	{ " Exit", HELP_SUBMENU_EXIT, CWinExitFn },
};
WINDOW_MENU Menu = { std::span(Item)};
} // namespace Cfg

namespace Rep {
template <int Stage> bool FnStg(INPUT_BITS key);

constexpr auto FnStgEx = FnStg<GRAPH_ID_EXSTAGE>;
WINDOW_CHOICE Item[] = {
	{ " Stage 1 デモ再生", "ステージ１のリプレイ", FnStg<1> },
	{ " Stage 2 デモ再生", "ステージ２のリプレイ", FnStg<2> },
	{ " Stage 3 デモ再生", "ステージ３のリプレイ", FnStg<3> },
	{ " Stage 4 デモ再生", "ステージ４のリプレイ", FnStg<4> },
	{ " Stage 5 デモ再生", "ステージ５のリプレイ", FnStg<5> },
	{ " Stage 6 デモ再生", "ステージ６のリプレイ", FnStg<6> },
	{ " ExStage デモ再生", "エキストラステージのリプレイ", FnStgEx },
	{ " Exit", HELP_SUBMENU_EXIT, CWinExitFn },
};
WINDOW_MENU Menu = { std::span(Item)};
} // namespace Rep

static bool FnGameStart(INPUT_BITS key);
static bool FnExStart(INPUT_BITS key);
static bool FnMusic(INPUT_BITS key);
static bool FnScore(INPUT_BITS key);
static void SetItem(bool tick = true);

WINDOW_LABEL Title = { "     Main Menu" };
WINDOW_CHOICE Item[] = {
	// { "   Game  Start",	"ゲームを開始します(使用不可)" },
	{ "   Game  Start", "ゲームを開始します", FnGameStart },
	{ "   Extra Start", "ゲームを開始します(Extra)", FnExStart },
	{ "   Replay", "リプレイを開始します", Rep::Menu },
	{ "   Config", "各種設定を変更します", Cfg::Menu },
	{ "   Score", "スコアの表示をします", FnScore },
	{ "   Music", "音楽室に入ります", FnMusic },
	{ "   Exit", "ゲームを終了します", CWinExitFn },
};
WINDOW_MENU Menu = { std::span(Item), SetItem, &Title };
static auto& ItemMusic = Item[5];
} // namespace Main

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
WINDOW_SYSTEM MainWindow = { Main::Menu };
WINDOW_SYSTEM ExitWindow = { ExitMenu };
WINDOW_SYSTEM ContinueWindow = { ContinueMenu };
WINDOW_SYSTEM BGMPackWindow = { BGMPackMenu.Menu };



// メインメニューの初期化 //
void InitMainWindow(void)
{
	using namespace Main;
	using namespace Cfg::Grp;

#ifdef SUPPORT_GRP_API
	const auto grp_api_count = GrpBackend_APICount();
	if(grp_api_count >= 2) {
		assert(grp_api_count <= std::size(API::Item));
		auto *menu_p = API::Menu.ItemPtr;

		*(menu_p++) = &API::ItemDef;
		for(const auto i : std::views::iota(0, grp_api_count)) {
			const Narrow::string_view name = GrpBackend_APIName(i);
			assert(!name.empty());
			API::Item[i] = { name.data(), HELP_API_SPECIFIC, API::FnOverride };
			*(menu_p++) = &API::Item[i];
		}
		*(menu_p++) = &SubmenuExitItem;
		API::Menu.NumItems = std::distance(API::Menu.ItemPtr, menu_p);
	} else {
		ItemAPI.Flags |= WINDOW_FLAGS::DISABLED;
	}
#endif

	MainWindow.Init(140);

	// エキストラステージが選択できる場合には発生！ //
	if(ConfigDat.ExtraStgFlags.v) {
		Cfg::Menu.NumItems = 6;
		Cfg::Menu.ItemPtr[4] = &Cfg::Item[4];
		Cfg::Menu.ItemPtr[5] = &Cfg::Item[5];
	}
	else{
		Cfg::Menu.NumItems = 5;
		Cfg::Menu.ItemPtr[4] = &Cfg::Item[5];
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

static void Main::Cfg::Dif::FnPlayerStock(int_fast8_t delta)
{
	RingStep(ConfigDat.PlayerStock.v, delta, 0, STOCK_PLAYER_MAX);
}

static void Main::Cfg::Dif::FnBombStock(int_fast8_t delta)
{
	RingStep(ConfigDat.BombStock.v, delta, 0, STOCK_BOMB_MAX);
}

static void Main::Cfg::Dif::FnDifficulty(int_fast8_t delta)
{
	RingStep(ConfigDat.GameLevel.v, delta, GAME_EASY, GAME_LUNATIC);
}

#ifdef PBG_DEBUG
static void Main::Cfg::Dif::FnMsgDisplay(int_fast8_t)
{
	DebugDat.MsgDisplay = !DebugDat.MsgDisplay;
}

static void Main::Cfg::Dif::FnStgSelect(int_fast8_t delta)
{
	RingStep(DebugDat.StgSelect, delta, 1, STAGE_MAX);
}

static void Main::Cfg::Dif::FnHit(int_fast8_t)
{
	DebugDat.Hit = !DebugDat.Hit;
}

static void Main::Cfg::Dif::FnDemo(int_fast8_t)
{
	DebugDat.DemoSave = !DebugDat.DemoSave;
}
#endif // PBG_DEBUG

static void Main::Cfg::Grp::API::FnDef(int_fast8_t)
{
	XGrpTry([](auto& params) {
		params.api = -1;
	});
}

static void Main::Cfg::Grp::FnChgDevice(int_fast8_t delta)
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

static void Main::Cfg::Grp::API::FnOverride(int_fast8_t)
{
	XGrpTry([](auto& params) {
		params.api = (MainWindow.Select[MainWindow.SelectDepth] - 1);
	});
}

static void Main::Cfg::Grp::FnDisp(int_fast8_t)
{
	XGrpTryCycleDisp();
}

static void Main::Cfg::Grp::FnFSMode(int_fast8_t)
{
	XGrpTry([](auto& params) {
		params.flags ^= GRAPHICS_PARAM_FLAGS::FULLSCREEN_EXCLUSIVE;
	});
}

static void Main::Cfg::Grp::FnScale(int_fast8_t delta)
{
	XGrpTryCycleScale(delta, true);
}

static void Main::Cfg::Grp::FnScMode(int_fast8_t)
{
	XGrpTryCycleScMode();
}

static void Main::Cfg::Grp::FnSkip(int_fast8_t delta)
{
	RingStep(ConfigDat.FPSDivisor.v, delta, 0, FPS_DIVISOR_MAX);
	Grp_FPSDivisor = ConfigDat.FPSDivisor.v;
}

static void Main::Cfg::Grp::FnBpp(int_fast8_t delta)
{
	XGrpTry([delta](auto& params) {
		params.bitdepth = ConfigDat.BitDepth.v.cycle(delta < 0);
	});
}

static void Main::Cfg::Grp::FnWinLocate(int_fast8_t delta)
{
	static constexpr uint8_t flags[3] = {
		0, GRPF_WINDOW_UPPER, GRPF_MSG_DISABLE
	};
	auto it = std::ranges::find_if(flags, [](auto f) {
		return ((ConfigDat.GraphFlags.v & GRPF_ORIG_MASK) == f);
	});
	const auto i = ((it != std::end(flags)) ? std::distance(flags, it) : 0);

	ConfigDat.GraphFlags.v &= ~GRPF_ORIG_MASK;
	if(delta < 0) {
		ConfigDat.GraphFlags.v |= flags[(i + 2) % std::size(flags)];
	} else {
		ConfigDat.GraphFlags.v |= flags[(i + 1) % std::size(flags)];
	}
}

static void Main::Cfg::Snd::FnSE(int_fast8_t)
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

static void Main::Cfg::Snd::FnBGM(int_fast8_t)
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

static void Main::Cfg::Snd::FnSEVol(int_fast8_t delta)
{
	ConfigDat.SEVolume.v = std::clamp(
		(ConfigDat.SEVolume.v + delta), 0, int{ VOLUME_MAX }
	);
	Snd_UpdateVolumes();
}

static void Main::Cfg::Snd::FnBGMVol(int_fast8_t delta)
{
	ConfigDat.BGMVolume.v = std::clamp(
		(ConfigDat.BGMVolume.v + delta), 0, int{ VOLUME_MAX }
	);
	BGM_UpdateVolume();
}

static void Main::Cfg::Snd::FnBGMPack(int_fast8_t)
{
	if(!BGM_PacksAvailable()) {
		URLOpen(BGMPack::SOUNDTRACK_URL);
	} else {
		BGMPack::Open();
	}
}

static void Main::Cfg::Snd::FnBGMGain(int_fast8_t)
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

	void Open(void)
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

		// `WINDOW_FLAGS::DISABLED` is set by default if a `WINDOW_CHOICE` is
		// constructed without text, so we must indeed overwrite the entire
		// flag field.
		ret.Flags = ((generated == SelAtOpen)
			? WINDOW_FLAGS::HIGHLIGHT
			: WINDOW_FLAGS::NONE
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
		if(Input_IsOK(key)) {
			if(selected == SelDownload()) {
				URLOpen(BGMPack::SOUNDTRACK_URL);
			} else {
				if(selected == SelNone()) {
					ConfigDat.BGMPack.v.clear();
				} else {
					ConfigDat.BGMPack.v = Packs[selected - 1];
				}
				Main::Cfg::Snd::SetItem();
				BGM_PackSet(ConfigDat.BGMPack.v);
			}
			return false;
		}
		return true;
	}
}

static void Main::Cfg::Snd::Mid::FnDev(int_fast8_t delta)
{
	if(BGM_Enabled()) {
		BGM_ChangeMIDIDevice(delta);
	}
}

static void Main::Cfg::Snd::Mid::FnFixes(int_fast8_t)
{
	const auto flags = (ConfigDat.MidFlags.v ^ MID_FLAGS::FIX_SYSEX_BUGS);
	ConfigDat.MidFlags.v = Mid_SetFlags(flags);
}

static void Main::Cfg::Inp::FnMsgSkip(int_fast8_t)
{
	if(ConfigDat.InputFlags.v & INPF_Z_MSKIP_ENABLE) {
		ConfigDat.InputFlags.v &= (~INPF_Z_MSKIP_ENABLE);
	} else {
		ConfigDat.InputFlags.v |= INPF_Z_MSKIP_ENABLE;
	}
}

static void Main::Cfg::Inp::FnZSpeedDown(int_fast8_t)
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
	} else if(Input_IsOK(key)) {
		GameReplayInit(stage);
	}
	return true;
}


template <int Stage> bool Main::Rep::FnStg(INPUT_BITS key)
{
	return RFnStg(Stage, key);
}



static bool Main::FnGameStart(INPUT_BITS key)
{
	if(Input_IsOK(key)) {
		//EndingInit();
		WeaponSelectInit(false);
	}
	return true;
}

static bool Main::FnExStart(INPUT_BITS key)
{
	if(Input_IsOK(key)) {
		if(ConfigDat.ExtraStgFlags.v) {
			WeaponSelectInit(true);
		}
	}
	return true;
}

static bool Main::FnScore(INPUT_BITS key)
{
	if(Input_IsOK(key)) {
		ScoreNameInit();
	}
	return true;
}

static bool Main::FnMusic(INPUT_BITS key)
{
	if(Input_IsOK(key)) {
		MWinForceClose();
		MusicRoomInit();
	}
	return true;
}

static bool ExitFnYes(INPUT_BITS key)
{
	if(Input_IsOK(key)) {
		GameExit();
		return false;
	}
	return true;
}

static bool ExitFnNo(INPUT_BITS key)
{
	if(Input_IsOK(key)) {
		GameRestart();
		return false;
	}
	return true;
}

static bool ContinueFnYes(INPUT_BITS key)
{
	if(Input_IsOK(key)) {
		GameContinue();
		return false;
	}
	return true;
}

static bool ContinueFnNo(INPUT_BITS key)
{
	if(Input_IsOK(key)) {
		NameRegistInit(true);
		// GameExit();
		return false;
	}
	return true;
}

static bool InpFnPad(INPUT_PAD_BUTTON& config_pad, INPUT_BITS key)
{
	key &= (~Pad_Data);
	const auto temp = Key_PadSingle();
	if(temp) {
		config_pad = temp.value();
		Main::Cfg::Inp::SetItem();
	}
	return !Input_IsOK(key);
}


template <INPUT_PAD_BUTTON& ConfigPad> bool Main::Cfg::Inp::Pad::Fn(
	INPUT_BITS key
)
{
	return InpFnPad(ConfigPad, key);
}


static void Main::Cfg::Rep::FnStgSelect(int_fast8_t delta)
{
	RingStep(ConfigDat.StageSelect.v, delta, 1, STAGE_MAX);
}


static void Main::Cfg::Rep::FnSave(int_fast8_t)
{
	ConfigDat.StageSelect.v = ((ConfigDat.StageSelect.v) ? 0 : 1);
}

static void Main::SetItem(bool)
{
	ItemMusic.SetActive(BGM_Enabled());
}


static void Main::Cfg::Rep::SetItem(bool)
{
	if(0 == ConfigDat.StageSelect.v) {
		sprintf(Title[0], "ReplaySave  %s", CHOICE_OFF_ON[false]);
		strcpy(Title[1], "StageSelect [無 効]");
	}
	else{
		sprintf(Title[0], "ReplaySave  %s", CHOICE_OFF_ON[true]);
		sprintf(Title[1], "StageSelect [  %d  ]", ConfigDat.StageSelect.v);
	}
}


static void Main::Cfg::Dif::SetItem(bool)
{
	static constexpr const char *const dif[4] = {
		" Easy  ",
		" Normal",
		" Hard  ",
		"Lunatic",
	};
/*
	{Title[3], "[DebugMode] 画面に情報を表示するか", FnMsgDisplay,0,0},
	{Title[4], "[DebugMode] ステージセレクト", FnStgSelect,0,0},
	{Title[5], "[DebugMode] 当たり判定", FnHit,0,0},
*/
	// +1 に注意
	sprintf(Title[0], "PlayerStock [ %d ]", (ConfigDat.PlayerStock.v + 1));
	sprintf(Title[1], "BombStock   [ %d ]", ConfigDat.BombStock.v);
	sprintf(Title[2], "Difficulty[%s]", dif[ConfigDat.GameLevel.v]);

#ifdef PBG_DEBUG
	sprintf(Title[3], "DebugOut  %s", CHOICE_OFF_ON[DebugDat.MsgDisplay]);
	sprintf(Title[4], "StgSelect [  %d  ]", DebugDat.StgSelect);
	sprintf(Title[5], "Hit       %s", CHOICE_OFF_ON[DebugDat.Hit]);
	sprintf(Title[6], "DemoSave  %s", CHOICE_OFF_ON[DebugDat.DemoSave]);
#endif
}

static void Main::Cfg::Grp::SetItem(bool)
{
	const auto params = ConfigDat.GraphicsParams();

	static constexpr auto aspect = (GRP_RES / std::gcd(GRP_RES.w, GRP_RES.h));
	static constexpr const char *DISPLAY_MODES[] = {
		"  Window  ",
		"Fullscreen",
	};
	static constexpr const char *FULLSCREEN_MODES[] = {
		"Borderless",
		"Exclusive ",
	};
	static constexpr ENUMARRAY<
		std::pair<const char *, const char *>, GRAPHICS_FULLSCREEN_FIT
	> FITS = {
		std::pair{ "%s[Integer]", "Use largest integer %d:%d resolution" },
		std::pair{ "%s[  %d:%d  ]", "Use largest fractional %d:%d resolution" },
		std::pair{ "%s[Stretch]", "Use aspect ratio of display" },
	};
	static constexpr std::tuple<const char*, bool> SCALE_MODES[] = {
		{ "FrameBuf", false },
		{ "Geometry", false },
		{ "--------", true },
	};
	static constexpr const char *const UorD[3] = {
		"上のほう",
		"下のほう",
		"描画せず",
	};
	static constexpr const char *const FRate[4] = {
		"おまけ",
		"60Fps",
		"30Fps",
		"20Fps",
	};
	const auto u_or_d = ((ConfigDat.GraphFlags.v & GRPF_MSG_DISABLE)
		? 2
		: ((ConfigDat.GraphFlags.v & GRPF_WINDOW_UPPER) ? 0 : 1)
	);
	const auto dev = GrpBackend_DeviceName(ConfigDat.DeviceID.v);

	const auto fs = params.FullscreenFlags();
	const auto in_borderless_fullscreen = (fs.fullscreen && !fs.exclusive);

#ifdef SUPPORT_GRP_SCALING
	const auto scale_4x = params.Scale4x();
	const auto [scale_var1, scale_var2] = (in_borderless_fullscreen
		? std::pair<uint8_t, uint8_t>(aspect.w, aspect.h)
		: std::pair<uint8_t, uint8_t>((scale_4x / 4u), ((scale_4x % 4u) * 25u))
	);
	const auto scale_res = params.ScaledRes();
	const auto [scale_fmt, scale_label] = (in_borderless_fullscreen
		? std::pair(FITS[fs.fit].first, "FullScrFit")
		: std::pair((scale_4x ? "%s[%3u.%02ux ]" : "%s[ Screen ]"), "ScaleFact")
	);

	const auto [sc_mode_label, sc_mode_disabled] = (
		(scale_4x == 4) ? SCALE_MODES[2] :
		params.ScaleGeometry() ? SCALE_MODES[1] :
		SCALE_MODES[0]
	);

	ItemScale.SetActive(!(fs.fullscreen && fs.exclusive));
	EnumFlagSet(ItemScMode.Flags, WINDOW_FLAGS::DISABLED, sc_mode_disabled);
#endif

	// clang-format off
	sprintf(TitleDevice, "Device   [%.7s]", dev.data());
#ifdef SUPPORT_GRP_WINDOWED
	sprintf(TitleDisp,   "Display[%s]", DISPLAY_MODES[fs.fullscreen]);
	sprintf(TitleFSMode, "FullScr[%s]", FULLSCREEN_MODES[fs.exclusive]);
#endif
#ifdef SUPPORT_GRP_SCALING
	sprintf(TitleScale,  scale_fmt, scale_label, scale_var1, scale_var2);
	sprintf(TitleScMode, "ScaleMode[%s]", sc_mode_label);
#endif
	sprintf(TitleSkip,   "FrameRate[ %s ]", FRate[ConfigDat.FPSDivisor.v]);
#ifdef SUPPORT_GRP_BITDEPTH
	sprintf(TitleBpp,    "BitDepth [ %dBit ]", ConfigDat.BitDepth.v.value());
#endif
	sprintf(TitleMsg,    "MsgWindow[%s]", UorD[u_or_d]);
	// clang-format on

	// Help strings
	// ------------
#ifdef SUPPORT_GRP_WINDOWED
	const auto fs_mode_help_fmt = (fs.exclusive
		? "Fullscreen changes resolution to %dx%d"
		: "Fullscreen uses a display-sized window"
	);
	sprintf(HelpFSMode, fs_mode_help_fmt, GRP_RES.w, GRP_RES.h);
#endif

#ifdef SUPPORT_GRP_SCALING
	if(in_borderless_fullscreen) {
		sprintf(HelpScale, FITS[fs.fit].second, aspect.w, aspect.h);
	} else if(scale_4x == 0) {
		strcpy(HelpScale, "Game scales to fit the display");
	} else {
		const auto [fmt, res] = ((scale_4x == 4)
			? std::pair("Window size is %d×%d, not scaling", GRP_RES)
			: std::pair("Game is scaled to %d×%d", scale_res)
		);
		sprintf(HelpScale, fmt, res.w, res.h);
	}

	auto [sc_mode_fmt, sc_mode_res] = (params.ScaleGeometry()
		? std::pair("Render at %d×%d, scale each draw call", scale_res)
		: std::pair("Render at %d×%d, then scale framebuffer", GRP_RES)
	);
	sprintf(HelpScMode, sc_mode_fmt, sc_mode_res.w, sc_mode_res.h);
#endif
	// ------------
}

static void Main::Cfg::Grp::Screenshot::SetItem(bool)
{
}

#ifdef SUPPORT_GRP_API
static void Main::Cfg::Grp::API::SetItem(bool)
{
	const bool is_def_api = (ConfigDat.GraphicsAPI.v < 0);

	// Since [ConfigDat.GraphicsAPI.v] can be negative, we must find the
	// active entry via string comparison.
	const Narrow::string_view api_active = GrpBackend_APIName(
		ConfigDat.GraphicsAPI.v
	);

	ItemDef.SetActive(!is_def_api);
	for(auto& api : Item | std::views::take(GrpBackend_APICount())) {
		const auto is_selected = !strcmp(api_active.data(), api.Title.ptr);
		EnumFlagSet(api.Flags, WINDOW_FLAGS::HIGHLIGHT, is_selected);
	}

	sprintf(TitleDef, "UseDefault  %s", CHOICE_OFF_ON[is_def_api]);
}
#endif

static void Main::Cfg::Snd::SetItem(bool)
{
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
	ItemSEVol.SetActive(sound_active);
	ItemBGMVol.SetActive(bgm_active);
	ItemBGMGain.SetActive(bgm_active && BGM_HasGainFactor());

	sprintf(TitleSE,      "Sound  [%s]", CHOICE_USE[!sound_active]);
	sprintf(TitleBGM,     "BGM    [%s]", CHOICE_USE[!bgm_active]);
	sprintf(TitleSEVol,   "SoundVolume [ %3d ]", ConfigDat.SEVolume.v);
	sprintf(TitleBGMVol,  "BGMVolume   [ %3d ]", ConfigDat.BGMVolume.v);
	sprintf(TitleBGMGain, "BGMVolNormalize%s", norm_choice);
	if(!BGM_PacksAvailable()) {
		sprintf(TitleBGMPack, "BGMPack[ Download ]");
		ItemBGMPack.Help = BGMPack::HELP_DOWNLOAD;
	} else if(ConfigDat.BGMPack.v.empty()) {
		sprintf(TitleBGMPack, "BGMPack[%s]", CHOICE_USE[1]);
		ItemBGMPack.Help = BGMPack::HELP_SET;
	} else {
		sprintf(TitleBGMPack, "BGMPack[   ....   ]");
		ItemBGMPack.Help = BGMPack::HELP_SET;
	}
}

#ifdef SUPPORT_MIDI_BACKEND
static void Main::Cfg::Snd::Mid::SetItem(bool tick)
{
	static int now;
	char buf[1000];
	static uint8_t time;

	const auto maybe_dev = MidBackend_DeviceName();
	if(maybe_dev) {
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
		sprintf(TitlePort, ">%.18s", (buf + now));
	} else {
		strcpy(TitlePort, ">");
	}
	EnumFlagSet(ItemPort.Flags, WINDOW_FLAGS::DISABLED, !maybe_dev);
	const auto fixes = !!(ConfigDat.MidFlags.v & MID_FLAGS::FIX_SYSEX_BUGS);

	sprintf(TitleFixes, "SC88ProFXCompat%s", CHOICE_OFF_ON_NARROW[fixes]);
}
#endif

static void Main::Cfg::Inp::SetItem(bool)
{
	const auto skip = ((ConfigDat.InputFlags.v & INPF_Z_MSKIP_ENABLE) != 0);
	const auto down = ((ConfigDat.InputFlags.v & INPF_Z_SPDDOWN_ENABLE) != 0);

	sprintf(Title[0], "Z-MessageSkip[%s]", (skip ? "ＯＫ" : "禁止"));
	sprintf(Title[1], "Z-SpeedDown  [%s]", (down ? "ＯＫ" : "禁止"));
}

static void Main::Cfg::Inp::Pad::SetItem(bool)
{
	constexpr LABELS<4> labels = {{ "Shot", "Bomb", "SpeedDown", "ESC" }};
	auto set = [](char* buf, Narrow::string_view label, INPUT_PAD_BUTTON v) {
		if(v > 0) {
			sprintf(buf, "%-*s[Button%2d]", labels.w, label.data(), v);
		} else {
			sprintf(buf, "%-*s[--------]", labels.w, label.data());
		}
	};
	set(Title[0], labels.str[0], ConfigDat.PadTama.v);
	set(Title[1], labels.str[1], ConfigDat.PadBomb.v);
	set(Title[2], labels.str[2], ConfigDat.PadShift.v);
	set(Title[3], labels.str[3], ConfigDat.PadCancel.v);
}
