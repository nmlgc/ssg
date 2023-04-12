/*
 *   Config data
 *
 */

#pragma once

#include "platform/file.h"
#include "CONFIG.H"

///// Constants /////
constexpr auto DBG_FN = _PATH("秋霜DBG.DAT");

// Arrays of configuration options for simple serialization
template <CONFIG_OPTION... Options> class CONFIG_OPTION_ARRAY {
private:
	const std::tuple<Options&...> options;

public:
	constexpr CONFIG_OPTION_ARRAY(Options&... options)
		: options(options...) {
	}

	const size_t Size() const {
		return std::apply([](const auto&... opt) {
			return (... + opt.Size());
		}, options);
	}

	bool Read(BYTE_BUFFER_CURSOR<const uint8_t>& cursor) const {
		return std::apply([&cursor](auto&... opt) {
			return (... && opt.Read(cursor));
		}, options);
	}

	bool Write(BYTE_BUFFER_CURSOR<uint8_t>& cursor) const {
		return std::apply([&cursor](auto&... opt) {
			return (... && opt.Write(cursor));
		}, options);
	}

	// Recursively writes the options into a buffer, to be then saved into the
	// configuration file.
	BYTE_BUFFER_OWNED Serialize() const {
		BYTE_BUFFER_OWNED ret = { Size() };
		auto cursor = ret.cursor_mut();
		return (!Write(cursor) ? nullptr : std::move(ret));
	}
};

// On-disk config file versions
// ----------------------------

// Original 秋霜CFG.DAT
const struct {
	static constexpr auto FN = _PATH("秋霜CFG.DAT");

	static inline CONFIG_OPTION_VALUE<uint32_t> CheckSum;
	static inline CONFIG_OPTION_VALUE<uint8_t> PaddingByte;

	static constexpr CONFIG_OPTION_ARRAY Options = {
		ConfigDat.GameLevel,
		ConfigDat.PlayerStock,
		ConfigDat.BombStock,
		ConfigDat.DeviceID,
		ConfigDat.BitDepth,
		ConfigDat.FPSDivisor,
		ConfigDat.GraphFlags,
		ConfigDat.SoundFlags,
		ConfigDat.InputFlags,
		ConfigDat.DebugFlags,
		ConfigDat.PadTama,
		ConfigDat.PadBomb,
		ConfigDat.PadShift,
		ConfigDat.PadCancel,
		ConfigDat.ExtraStgFlags,
		PaddingByte,
		CheckSum,

		// ワケ有りでここにいるのです(チェックサムの範囲外)
		ConfigDat.StageSelect,

		PaddingByte,
		PaddingByte,
		PaddingByte,
	};

	static uint32_t CheckSumCalculate() {
		uint32_t ret = 0;
		ret += (ConfigDat.BitDepth.v.value() << 1);
		ret += (ConfigDat.BombStock.v << 3);
		ret += (ConfigDat.DeviceID.v << 2);
		ret += (ConfigDat.FPSDivisor.v << 6);
		ret += (ConfigDat.ExtraStgFlags.v << 5);
		ret += (ConfigDat.GameLevel.v << 2);
		ret += (ConfigDat.GraphFlags.v << 1);
		ret += (ConfigDat.InputFlags.v << 3);
		ret += (ConfigDat.PadBomb.v << 6);
		ret += (ConfigDat.PadCancel.v << 3);
		ret += (ConfigDat.PadShift.v << 1);
		ret += (ConfigDat.PadTama.v << 4);
		ret += (ConfigDat.PlayerStock.v << 5);
		ret += (ConfigDat.SoundFlags.v << 2);
		return ret;
	}
} VERSION_ORIGINAL;
// ----------------------------

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

	const auto& version = VERSION_ORIGINAL;
	const auto loaded_buf = FileLoad(version.FN);
	auto loaded_cursor = loaded_buf.cursor();
	version.Options.Read(loaded_cursor);
}

// コンフィグの内容を保存する //
extern void ConfigSave(void)
{
	const auto& version = VERSION_ORIGINAL;
	version.CheckSum.v = version.CheckSumCalculate();
	const auto buf = version.Options.Serialize();
	if(buf) {
		FileWrite(version.FN, buf.cursor());
	}

#ifdef PBG_DEBUG
	FileWrite(DBG_FN, DebugDat);
#endif
}
