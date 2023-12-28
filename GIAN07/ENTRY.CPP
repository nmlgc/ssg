/*
 *   Generic, cross-platform subsystem initialization and cleanup
 *
 */

#include "GIAN07/ENTRY.H"
#include "GIAN07/CONFIG.H"
#include "GIAN07/GAMEMAIN.H"
#include "GIAN07/LOADER.H"
#include "DirectXUTYs/DD_UTY.H"
#include "DirectXUTYs/PBGMIDI.H"
#include "platform/input.h"
#include "platform/path.h"
#include "platform/snd.h"
#include "game/coords.h"
#include "game/screenshot.h"
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
const int SND_X_MID = PixelToWorld(320);

const int SND_X_PER_DECIBEL = PixelToWorld(25);
// -----------------------------------------------------------

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

	// サウンドの初期化 //
	if(ConfigDat.SoundFlags.v & SNDF_WAVE_ENABLE) {
		// 何らかの理由で使用できなければ、Disable とする //
		if(!SndInit()) {
			ConfigDat.SoundFlags.v &= (~SNDF_WAVE_ENABLE);
		} else if(!LoadSound()) {
			ConfigDat.SoundFlags.v &= (~SNDF_WAVE_ENABLE);
			SndCleanup();
		}
	}

	// ＭＩＤＩの初期化 //
	// MIDFN_MIDLOOP を使用すべきか //
	if(ConfigDat.SoundFlags.v & SNDF_MIDI_ENABLE) {
		if(!Mid_Start(MIDFN_CALLBACK, MIDPL_NORM)) {
			ConfigDat.SoundFlags.v &= (~SNDF_MIDI_ENABLE);
		}
	}

	// Call a different *Init() function here to quickly test a different //
	// game state. //
	if(!SProjectInit()) {
		return false;
	}

	ScreenshotSetPrefix(u8"秋霜");

	return true;
}

void XCleanup(void)
{
	ConfigSave();
	DxObj.Cleanup();
	SndCleanup();
	Mid_Stop();
	Mid_End();
	Key_End();
}

