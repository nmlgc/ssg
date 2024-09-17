/*
 *   Generic, cross-platform subsystem initialization and cleanup
 *
 */

#include "GIAN07/ENTRY.H"
#include "GIAN07/CONFIG.H"
#include "GIAN07/FONTUTY.H"
#include "GIAN07/GAMEMAIN.H"
#include "GIAN07/LOADER.H"
#include "platform/graphics_backend.h"
#include "platform/input.h"
#include "platform/path.h"
#include "game/bgm.h"
#include "game/debug.h"
#include "game/frame.h"
#include "game/snd.h"

// Volume controls
// ---------------

const VOLUME& Mid_Volume = ConfigDat.BGMVolume.v;
const VOLUME& Snd_VolumeBGM = ConfigDat.BGMVolume.v;
const VOLUME& Snd_VolumeSE = ConfigDat.SEVolume.v;
// ---------------

// MUSIC.DAT loaders
// -----------------

bool (*const BGM_MidLoadOriginal)(unsigned int id) = LoadMusic;
bool (*const BGM_MidLoadBuffer)(BYTE_BUFFER_OWNED) = LoadMIDIBuffer;
bool (*const BGM_MidLoadByHash)(const HASH& hash) = LoadMusicByHash;
// -----------------

// Pad bindings
// ------------

static constexpr std::array<INPUT_PAD_BINDING, 4> PadBindings = { {
	{ ConfigDat.PadTama.v, KEY_TAMA },
	{ ConfigDat.PadBomb.v, KEY_BOMB },
	{ ConfigDat.PadShift.v, KEY_SHIFT },
	{ ConfigDat.PadCancel.v, KEY_ESC },
} };
std::span<const INPUT_PAD_BINDING> Key_PadBindings = PadBindings;
// ------------

bool XInit(void)
{
	std::filesystem::current_path(PathForData());

	DebugSetup();

	// コンフィグをロードする //
	ConfigLoad();
	Grp_FPSDivisor = ConfigDat.FPSDivisor.v;

	// キーボード(JoyPad)入力を受け付ける //
	Key_Start();

	// コンフィグ依存の初期化処理
	if(!GrpBackend_Enum()) {
		return false;
	}

	// グラフィックの初期化 //
	const auto maybe_params = Grp_InitOrFallback(ConfigDat.GraphicsParams());
	if(!maybe_params) {
		return false;
	}
	ConfigDat.GraphicsParamsApply(maybe_params.value().live);
	GrpBackend_SetClip(GRP_RES_RECT);

	// ＢＧＭの初期化 //
	if(ConfigDat.SoundFlags.v & SNDF_BGM_ENABLE) {
		BGM_Init();
	}
	if(!BGM_PackSet(ConfigDat.BGMPack.v)) {
		ConfigDat.BGMPack.v.clear();
	}
	BGM_SetGainApply(!(ConfigDat.SoundFlags.v & SNDF_BGM_NOT_VOL_NORM));
	Grp_SetScreenshotPrefix(u8"秋霜");
	LoaderInit();
	return true;
}

void XCleanup(void)
{
	ConfigSave();
	GrpBackend_Cleanup();
	BGM_Cleanup();
	Snd_Cleanup();
	Key_End();
}

void XGrpTry(const GRAPHICS_PARAMS& prev, const GRAPHICS_PARAMS& params)
{
	if(prev == params) {
		return;
	}
	auto maybe_result = Grp_Init(prev, params);
	if(!maybe_result) {
		// Try resetting to the previous configuration, or, if necessary,
		// attempt anything to get graphics back working again.
		maybe_result = Grp_InitOrFallback(prev);
	}
	if(maybe_result) {
		const auto& result = maybe_result.value();
		TextObj.WipeBeforeNextRender();
		ConfigDat.GraphicsParamsApply(result.live);
		if(result.reload_surfaces) {
			ReloadGraph();
		}

		// TODO: Assumes that 8-bit mode only ever calls this function within
		// the main menu.
		GrpSurface_PaletteApplyToBackend(SURFACE_ID::TITLE);
	}
}

bool GameFrame(void)
{
	bool quit = false;
	GameMain(quit);
	return !quit;
}
