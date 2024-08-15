/*
 *   Generic, cross-platform subsystem initialization and cleanup
 *
 */

#include "GIAN07/ENTRY.H"
#include "GIAN07/CONFIG.H"
#include "GIAN07/FONTUTY.H"
#include "GIAN07/GAMEMAIN.H"
#include "GIAN07/LOADER.H"
#include "DirectXUTYs/DD_UTY.H"
#include "platform/input.h"
#include "platform/path.h"
#include "game/bgm.h"
#include "game/debug.h"
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

bool XInit(void)
{
	std::filesystem::current_path(PathForData());

	DebugSetup();

	// コンフィグをロードする //
	ConfigLoad();

	// キーボード(JoyPad)入力を受け付ける //
	Key_Start();

	// コンフィグ依存の初期化処理
	if(!GrpEnum()) {
		return false;
	}

	// グラフィックの初期化 //
	if(!DxObj.Init(ConfigDat.DeviceID.v, ConfigDat.BitDepth.v)) {
		return false;
	}
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
	DxObj.Cleanup();
	BGM_Cleanup();
	Snd_Cleanup();
	Key_End();
}

void XGrpTry(GRAPHICS_PARAMS& params)
{
	if(DxObj.Init(params.device_id, params.bitdepth)) {
		TextObj.WipeBeforeNextRender();
		ConfigDat.GraphicsParamsApply(params);

		// TODO: Assumes that 8-bit mode only ever calls this function within
		// the main menu.
		GrpSurface_PaletteApplyToBackend(SURFACE_ID::TITLE);
	}
}
