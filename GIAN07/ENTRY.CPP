/*
 *   Generic, cross-platform subsystem initialization and cleanup
 *
 */

#include "GIAN07/ENTRY.H"
#include "GIAN07/CONFIG.H"
#include "GIAN07/GAMEMAIN.H"
#include "GIAN07/LOADER.H"
#include "DirectXUTYs/DD_UTY.H"
#include "platform/input.h"
#include "platform/path.h"
#include "game/bgm.h"
#include "game/coords.h"
#include "game/debug.h"
#include "game/screenshot.h"
#include "game/snd.h"
#include <filesystem>

// Mapping world coordinates to a position in the stereo field
// -----------------------------------------------------------
// The algorithm from the original game:
//
// • The [x] values are world coordinates (Q26.6, 64 units per pixel)
// • Subtract the center of the screen (in world coordinates) from [x]
// • Divide the result by (a scalar) 16
// • Directly pass that result to DirectSound, which interprets it as a panning
//   value with a unit of 1/100 dB
//
// By transforming the calculation to pixel space and full decibels, we end up
// with ((16 / 64) × 100) = 25 pixels per shifted decibel.

// Ｘ座標の中心のデフォルト値
constexpr int SND_X_MID = PixelToWorld(320);

constexpr int SND_X_PER_DECIBEL = PixelToWorld(25);
// -----------------------------------------------------------

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

	// ＢＧＭの初期化 //
	if(ConfigDat.SoundFlags.v & SNDF_BGM_ENABLE) {
		BGM_Init();
	}
	if(!BGM_PackSet(ConfigDat.BGMPack.v)) {
		ConfigDat.BGMPack.v.clear();
	}
	BGM_SetGainApply(!(ConfigDat.SoundFlags.v & SNDF_BGM_NOT_VOL_NORM));
	ScreenshotSetPrefix(u8"秋霜");
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

