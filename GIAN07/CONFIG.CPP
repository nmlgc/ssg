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

	// Default settings
	ConfigDat = {
		// 難易度関連 //
		.GameLevel   = GAME_NORMAL,
		.PlayerStock = 2,
		.BombStock   = 2,

		// グラフィック関連 //
		// The fact that the default bit depth is hardware-dependent is most of
		// the reason why we initialize this here rather than using default
		// initializers within the struct itself.
		.DeviceID   = 0,
		.BitDepth   = dxenum[0].BitDepthBest(),
		.FPSDivisor = 1, // 60Fps Mode
		.GraphFlags = 0,

		// サウンド・ＭＩＤＩ関連 //
		.SoundFlags = (SNDF_WAVE_ENABLE | SNDF_MIDI_ENABLE),

		// 入力関連 //
		.InputFlags = INPF_Z_MSKIP_ENABLE,

		// デバッグ関連 //
		.DebugFlags = 0,

		// パッド関連 //
		.PadTama   = 1,
		.PadBomb   = 2,
		.PadShift  = 0,
		.PadCancel = 0,

		.ExtraStgFlags = 0,

		// その他 //
		.StageSelect = 0,
	};

	const auto maybe_loaded = FileLoad<CONFIG_DATA>(CFG_FN);
	if(!maybe_loaded) {
		return;
	}
	const auto& loaded = maybe_loaded.value();

	if(loaded.CheckSumCalculate() != loaded.CheckSum) {
		return;
	};

	if(loaded.DeviceID < dxenum.size()) {
		ConfigDat.DeviceID = loaded.DeviceID;
	}

	// Quirk: Off by 2 and 1, respectively?!
	if(loaded.PlayerStock <= (STOCK_PLAYER_MAX + 2)) {
		ConfigDat.PlayerStock = loaded.PlayerStock;
	}
	if(loaded.BombStock <= (STOCK_BOMB_MAX + 1)) {
		ConfigDat.BombStock = loaded.BombStock;
	}

	if(dxenum[loaded.DeviceID].BitDepthSupported(loaded.BitDepth)) {
		ConfigDat.BitDepth = loaded.BitDepth;
	}

	if(loaded.FPSDivisor <= FPS_DIVISOR_MAX) {
		ConfigDat.FPSDivisor = loaded.FPSDivisor;
	}
	if(loaded.GameLevel <= GAME_LUNATIC) {
		ConfigDat.GameLevel = loaded.GameLevel;
	}

	if(!(loaded.GraphFlags & GRPF_MASK)) {
		ConfigDat.GraphFlags = loaded.GraphFlags;
	}
	if(!(loaded.SoundFlags & SNDF_MASK)) {
		ConfigDat.SoundFlags = loaded.SoundFlags;
	}
	if(!(loaded.InputFlags & INPF_MASK)) {
		ConfigDat.InputFlags = loaded.InputFlags;
	}
	if(!(loaded.DebugFlags & DBGF_MASK)) {
		ConfigDat.DebugFlags = loaded.DebugFlags;
	}

	// 32 is the WinMM joy button limit //
	if(loaded.PadTama   <= 32) {
		ConfigDat.PadTama = loaded.PadTama;
	}
	if(loaded.PadBomb   <= 32) {
		ConfigDat.PadBomb = loaded.PadBomb;
	}
	if(loaded.PadShift  <= 32) {
		ConfigDat.PadShift = loaded.PadShift;
	}
	if(loaded.PadCancel <= 32) {
		ConfigDat.PadCancel = loaded.PadCancel;
	}
	ConfigDat.ExtraStgFlags = loaded.ExtraStgFlags;

	if(loaded.StageSelect <= STAGE_MAX) {
		ConfigDat.StageSelect = loaded.StageSelect;
	}
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
