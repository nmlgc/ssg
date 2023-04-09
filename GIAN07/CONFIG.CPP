/*
 *   Config data
 *
 */

#pragma once

#include "platform/file.h"
#include "CONFIG.H"
#include "LEVEL.H"

///// Constants /////
constexpr auto CFG_FN = _PATH("秋霜CFG.DAT");
constexpr auto DBG_FN = _PATH("秋霜DBG.DAT");

///// Methods /////
uint32_t CONFIG_DATA::CheckSumCalculate() const
{
	uint32_t ret = 0;
	ret += (BitDepth.v.value() << 1);
	ret += (BombStock.v << 3);
	ret += (DeviceID.v << 2);
	ret += (FPSDivisor.v << 6);
	ret += (ExtraStgFlags.v << 5);
	ret += (GameLevel.v << 2);
	ret += (GraphFlags.v << 1);
	ret += (InputFlags.v << 3);
	ret += (PadBomb.v << 6);
	ret += (PadCancel.v << 3);
	ret += (PadShift.v << 1);
	ret += (PadTama.v << 4);
	ret += (PlayerStock.v << 5);
	ret += (SoundFlags.v << 2);
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
		DebugDat.Hit        = true;
		DebugDat.MsgDisplay = true;
		DebugDat.DemoSave   = false;
		DebugDat.StgSelect  = 1;
	} else {
		DebugDat = maybe_loaded.value();
	}
}
#endif

// コンフィグの内容を初期化する //
extern void ConfigLoad()
{
#ifdef PBG_DEBUG
	DebugInit();
#endif

	// Default settings
	ConfigDat = {
		// 難易度関連 //
		.GameLevel   = GAME_NORMAL,
		.PlayerStock = 2,
		.BombStock   = 2,

		// グラフィック関連 //
		.DeviceID   = 0,
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

	// These are validated by their respective subsystems.
	ConfigDat.DeviceID = loaded.DeviceID;
	ConfigDat.BitDepth = loaded.BitDepth;

	// Quirk: Off by 2 and 1, respectively?!
	if(loaded.PlayerStock.v <= (STOCK_PLAYER_MAX + 2)) {
		ConfigDat.PlayerStock = loaded.PlayerStock;
	}
	if(loaded.BombStock.v <= (STOCK_BOMB_MAX + 1)) {
		ConfigDat.BombStock = loaded.BombStock;
	}

	if(loaded.FPSDivisor.v <= FPS_DIVISOR_MAX) {
		ConfigDat.FPSDivisor = loaded.FPSDivisor;
	}
	if(loaded.GameLevel.v <= GAME_LUNATIC) {
		ConfigDat.GameLevel = loaded.GameLevel;
	}

	if(!(loaded.GraphFlags.v & GRPF_MASK)) {
		ConfigDat.GraphFlags = loaded.GraphFlags;
	}
	if(!(loaded.SoundFlags.v & SNDF_MASK)) {
		ConfigDat.SoundFlags = loaded.SoundFlags;
	}
	if(!(loaded.InputFlags.v & INPF_MASK)) {
		ConfigDat.InputFlags = loaded.InputFlags;
	}
	if(!(loaded.DebugFlags.v & DBGF_MASK)) {
		ConfigDat.DebugFlags = loaded.DebugFlags;
	}

	// 32 is the WinMM joy button limit //
	if(loaded.PadTama.v   <= 32) {
		ConfigDat.PadTama = loaded.PadTama;
	}
	if(loaded.PadBomb.v   <= 32) {
		ConfigDat.PadBomb = loaded.PadBomb;
	}
	if(loaded.PadShift.v  <= 32) {
		ConfigDat.PadShift = loaded.PadShift;
	}
	if(loaded.PadCancel.v <= 32) {
		ConfigDat.PadCancel = loaded.PadCancel;
	}
	ConfigDat.ExtraStgFlags = loaded.ExtraStgFlags;

	if(loaded.StageSelect.v <= STAGE_MAX) {
		ConfigDat.StageSelect = loaded.StageSelect;
	}
}

// コンフィグの内容を保存する //
extern void ConfigSave(void)
{
	ConfigDat.CheckSum = ConfigDat.CheckSumCalculate();
	FileWrite(CFG_FN, ConfigDat);

#ifdef PBG_DEBUG
	FileWrite(DBG_FN, DebugDat);
#endif
}
