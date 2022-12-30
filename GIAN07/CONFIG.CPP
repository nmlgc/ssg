/*
 *   Config data
 *
 */

#pragma once

#include "platform/file.h"
#include "CONFIG.H"
#include "LEVEL.H"

///// Constants /////
constexpr auto CFG_FN = "秋霜CFG.DAT";
constexpr auto DBG_FN = "秋霜DBG.DAT";

///// Methods /////
uint32_t CONFIG_DATA::CheckSumCalculate() const
{
	uint32_t ret = 0;
	ret += (BitDepth.value() << 1);
	ret += (BombStock << 3);
	ret += (DeviceID << 2);
	ret += (FPSDivisor << 6);
	ret += (ExtraStgFlags << 5);
	ret += (GameLevel << 2);
	ret += (GraphFlags << 1);
	ret += (InputFlags << 3);
	ret += (PadBomb << 6);
	ret += (PadCancel << 3);
	ret += (PadShift << 1);
	ret += (PadTama << 4);
	ret += (PlayerStock << 5);
	ret += (SoundFlags << 2);
	return ret;
}

///// [グローバル変数] /////
CONFIG_DATA ConfigDat;
#ifdef PBG_DEBUG
	DEBUG_DATA DebugDat;
#endif

///// [ 関数(非公開) ] /////
static void ConfigInit(const std::span<const XDDRAWENUM> dxenum)
{
	// 難易度関連 //
	ConfigDat.BombStock   = 2;
	ConfigDat.GameLevel   = GAME_NORMAL;
	ConfigDat.PlayerStock = 2;

	// グラフィック関連 //
	ConfigDat.DeviceID   = 0;
	ConfigDat.BitDepth   = dxenum[ConfigDat.DeviceID].BitDepthBest();
	ConfigDat.FPSDivisor = 1;		// 60Fps Mode
	ConfigDat.GraphFlags = 0;

	// サウンド・ＭＩＤＩ関連 //
	ConfigDat.SoundFlags = SNDF_WAVE_ENABLE|SNDF_MIDI_ENABLE;

	// 入力関連 //
	ConfigDat.InputFlags = INPF_Z_MSKIP_ENABLE;

	// デバッグ関連 //
	ConfigDat.DebugFlags = 0;

	// パッド関連 //
	ConfigDat.PadTama   = 1;
	ConfigDat.PadBomb   = 2;
	ConfigDat.PadShift  = 0;
	ConfigDat.PadCancel = 0;

	ConfigDat.ExtraStgFlags = 0;

	// その他 //
	ConfigDat.StageSelect = 0;
}

#ifdef PBG_DEBUG
static void DebugInit(void)
{
	const auto maybe_loaded = FileLoad<DEBUG_DATA>(DBG_FN);
	if(!maybe_loaded) {
		DebugDat.Hit        = TRUE;
		DebugDat.MsgDisplay = TRUE;
		DebugDat.DemoSave   = FALSE;
		DebugDat.StgSelect  = 1;
	} else {
		DebugDat = maybe_loaded.value();
	}
}
#endif

// コンフィグの内容を初期化する //
extern void ConfigLoad(const std::span<const XDDRAWENUM> dxenum)
{
#ifdef PBG_DEBUG
	DebugInit();
#endif
	if(dxenum.size() == 0) {
		return;
	}

	while(1){
		const auto maybe_loaded = FileLoad<CONFIG_DATA>(CFG_FN);
		if(!maybe_loaded) {
			break;
		}
		ConfigDat = maybe_loaded.value();

		if(ConfigDat.CheckSumCalculate() != ConfigDat.CheckSum) break;

		// 列挙したデバイス数と比較して異常ならここで修正する //
		// ここに来た場合は、ファイルが壊れているかビデオカードが抜かれたかの //
		// どちらかなので、強制的に break しない。                            //
		if(ConfigDat.DeviceID >= dxenum.size()) ConfigDat.DeviceID = 0;

		// Quirk: Off by 2 and 1, respectively?!
		if(ConfigDat.PlayerStock > (STOCK_PLAYER_MAX + 2)) break;
		if(ConfigDat.BombStock > (STOCK_BOMB_MAX + 1)) break;

		if(dxenum[ConfigDat.DeviceID].BitDepthSupported(ConfigDat.BitDepth) == false) break;

		if(ConfigDat.FPSDivisor > FPS_DIVISOR_MAX) break;
		if(ConfigDat.GameLevel > GAME_LUNATIC) break;
		if(ConfigDat.GraphFlags&GRPF_MASK) break;

		if(ConfigDat.SoundFlags&SNDF_MASK) break;

		if(ConfigDat.InputFlags&INPF_MASK) break;

		if(ConfigDat.DebugFlags&DBGF_MASK) break;

		// 32 is the WinMM joy button limit //
		if(ConfigDat.PadTama   > 32) break;
		if(ConfigDat.PadBomb   > 32) break;
		if(ConfigDat.PadShift  > 32) break;
		if(ConfigDat.PadCancel > 32) break;

		// これだけは、特別扱い //
		if(ConfigDat.StageSelect > STAGE_MAX) ConfigDat.StageSelect = 0;

		return;
	}

	ConfigInit(dxenum);
}

// コンフィグの内容を保存する //
extern void ConfigSave(void)
{
	FILE		*fp;

	fp = fopen(CFG_FN, "wb");
	if(fp==NULL) return;

	ConfigDat.CheckSum = ConfigDat.CheckSumCalculate();

	fwrite(&ConfigDat,sizeof(CONFIG_DATA),1,fp);
	fclose(fp);

#ifdef PBG_DEBUG
	fp = fopen(DBG_FN, "wb");
	if(fp==NULL) return;
	fwrite(&DebugDat,sizeof(DEBUG_DATA),1,fp);
	fclose(fp);
#endif
}
