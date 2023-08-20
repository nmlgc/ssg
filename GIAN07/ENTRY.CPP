/*
 *   Generic, cross-platform subsystem initialization and cleanup
 *
 */

#include "GIAN07/ENTRY.H"
#include "GIAN07/CONFIG.H"
#include "GIAN07/GAMEMAIN.H"
#include "GIAN07/LOADER.H"
#include "DirectXUTYs/DD_UTY.H"
#include "DirectXUTYs/DI_UTY.H"
#include "DirectXUTYs/DS_UTY.H"
#include "DirectXUTYs/PBGMIDI.H"

bool XInit(void)
{
	DebugSetup();

	// コンフィグをロードする //
	ConfigLoad();

	// キーボード(JoyPad)入力を受け付ける //
	Key_Start();

	// コンフィグ依存の初期化処理
	if(!GrpEnum()) {
		return false;
	}

	Key_PadConfig2(ConfigDat.PadTama.v,   KEY_TAMA);
	Key_PadConfig2(ConfigDat.PadBomb.v,   KEY_BOMB);
	Key_PadConfig2(ConfigDat.PadShift.v,  KEY_SHIFT);
	Key_PadConfig2(ConfigDat.PadCancel.v, KEY_ESC);

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

	GrpSetCaptureFilename("秋霜");

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

