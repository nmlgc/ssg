/*
 *   Config data
 *
 */

#include "CONFIG.H"
#include "game/bgm.h"
#include "platform/file.h"
#include "platform/window_backend.h"

///// Constants /////
constexpr auto DBG_FN = _PATH("秋霜DBG.DAT");

size_t CONFIG_OPTION_UTF8::Size() const
{
	return (sizeof(U32BE) + (v.size() * sizeof(decltype(v)::value_type)));
}

bool CONFIG_OPTION_UTF8::Read(BYTE_BUFFER_CURSOR<const uint8_t>& cursor)
{
	const auto maybe_len = cursor.next<U32BE>();
	if(!maybe_len) {
		return false;
	}
	const auto len = maybe_len.value()[0];
	const auto maybe_bytes = cursor.next<decltype(v)::value_type>(len);
	if(!maybe_bytes) {
		return false;
	}
	const auto bytes = maybe_bytes.value();
	v.resize_and_overwrite(len, [bytes](auto buf, size_t n) {
		std::ranges::copy(bytes, buf);
		return n;
	});
	return true;
}

bool CONFIG_OPTION_UTF8::Write(BYTE_BUFFER_CURSOR<uint8_t>& cursor) const
{
	if(v.size() > (std::numeric_limits<uint32_t>::max)()) {
		return false;
	}
	auto maybe_len = cursor.next<U32BE>();
	if(!maybe_len) {
		return false;
	}
	maybe_len.value()[0] = static_cast<U32BE>(v.size());
	const auto maybe_bytes = cursor.next<decltype(v)::value_type>(v.size());
	if(!maybe_bytes) {
		return false;
	}
	const auto bytes = maybe_bytes.value();
	std::ranges::copy(v, bytes.data());
	return true;
}

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

template <typename T> concept VERSION = requires(T t) {
	{ t.Options } -> CONFIG_OPTION;
};

// Original 秋霜CFG.DAT
const struct VERSION_ORIGINAL {
	static constexpr auto FN = _PATH("秋霜CFG.DAT");

	// The original build of the game only supported 8-bit and 16-bit. Anything
	// else would cause a validation failure, resetting the configuration and
	// re-locking the Extra Stage even if the checksum was correct.
	static inline CONFIG_OPTION_VALUE<BITDEPTH> BitDepth8Or16;

	// The original build of the game didn't support all our fullscreen/window
	// and scaling flags.
	static inline CONFIG_OPTION_VALUE<uint8_t> GraphFlags;

	// The original build of the game only supported the MIDI and WAVE activity
	// bits.
	static inline CONFIG_OPTION_VALUE<uint8_t> SoundFlags;

	static inline CONFIG_OPTION_VALUE<uint32_t> CheckSum;
	static inline CONFIG_OPTION_VALUE<uint8_t> PaddingByte;

	static constexpr CONFIG_OPTION_ARRAY Options = {
		ConfigDat.GameLevel,
		ConfigDat.PlayerStock,
		ConfigDat.BombStock,
		ConfigDat.DeviceID,
		BitDepth8Or16,
		ConfigDat.FPSDivisor,
		GraphFlags,
		SoundFlags,
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
		ret += (BitDepth8Or16.v.value() << 1);
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

const struct VERSION_00 {
	static constexpr auto FN = _PATH("SSG_V00.CFG");

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
		ConfigDat.StageSelect,
	};
} VERSION_00;

const struct VERSION_01 {
	static constexpr auto FN = _PATH("SSG_V01.CFG");

	static constexpr CONFIG_OPTION_ARRAY Options = {
		VERSION_00.Options,
		ConfigDat.SEVolume,
		ConfigDat.BGMVolume,
		ConfigDat.BGMPack,
	};
} VERSION_01;

const struct VERSION_02 {
	static constexpr auto FN = _PATH("SSG_V02.CFG");

	static constexpr CONFIG_OPTION_ARRAY Options = {
		VERSION_01.Options,
		ConfigDat.GraphicsAPI,
		ConfigDat.WindowScale4x,
		ConfigDat.WindowLeft,
		ConfigDat.WindowTop,
		ConfigDat.MidFlags,
	};
} VERSION_02;


// Must be sorted from the newest to the oldest version.
const auto VERSIONS = std::make_tuple(
	VERSION_02,
	VERSION_01,
	VERSION_00,
	VERSION_ORIGINAL
);

bool VersionLoad(const VERSION auto& version)
{
	const auto loaded_buf = FileLoad(version.FN);
	auto loaded_cursor = loaded_buf.cursor();
	return version.Options.Read(loaded_cursor);
}

bool VersionSave(const VERSION auto& version)
{
	const auto buf = version.Options.Serialize();
	return (buf && FileWrite(version.FN, buf.cursor()));
}
// ----------------------------

GRAPHICS_PARAMS CONFIG_DATA::GraphicsParams(void) const
{
	const auto flags_shifted = (GraphFlags.v >> GRPF_PARAM_SHIFT);
	return {
		.flags = static_cast<GRAPHICS_PARAM_FLAGS>(flags_shifted),
		.device_id = DeviceID.v,
		.api = GraphicsAPI.v,
		.window_scale_4x = WindowScale4x.v,
		.left = WindowLeft.v,
		.top = WindowTop.v,
		.bitdepth = BitDepth.v,
	};
}

void CONFIG_DATA::GraphicsParamsApply(const GRAPHICS_PARAMS& params)
{
	GraphFlags.v &= ~GRPF_PARAM_MASK;
	GraphFlags.v |= (std::to_underlying(params.flags) << GRPF_PARAM_SHIFT);
	DeviceID.v = params.device_id;
	GraphicsAPI.v = params.api;
	WindowScale4x.v = params.window_scale_4x;
	WindowLeft.v = params.left;
	WindowTop.v = params.top;
	BitDepth.v = params.bitdepth;
}

///// [グローバル変数] /////
CONFIG_DATA ConfigDat;
#ifdef PBG_DEBUG
	DEBUG_DATA DebugDat;
#endif

#ifdef PBG_DEBUG
static void DebugInit(void)
{
	const auto loaded_bytes = FileLoadInplace(
		{ reinterpret_cast<uint8_t *>(&DebugDat), sizeof(DebugDat) }, DBG_FN
	);
	if(loaded_bytes != sizeof(DebugDat)) {
		DebugDat.Hit        = true;
		DebugDat.MsgDisplay = true;
		DebugDat.DemoSave   = false;
		DebugDat.StgSelect  = 1;
	}
}
#endif

// コンフィグの内容を初期化する //
extern void ConfigLoad()
{
#ifdef PBG_DEBUG
	DebugInit();
#endif

	std::apply([](const auto&... version) {
		(... || VersionLoad(version));
	}, VERSIONS);

	if(VERSION_ORIGINAL.BitDepth8Or16.loaded) {
		ConfigDat.BitDepth = VERSION_ORIGINAL.BitDepth8Or16;
	}
}

// コンフィグの内容を保存する //
extern void ConfigSave(void)
{
	// Backwards compatibility with 秋霜CFG.DAT
	VERSION_ORIGINAL.BitDepth8Or16.v = ((ConfigDat.BitDepth.v.value() > 16)
		? BITDEPTHS::find(16)
		: ConfigDat.BitDepth.v
	);
	VERSION_ORIGINAL.GraphFlags.v = (ConfigDat.GraphFlags.v & GRPF_ORIG_MASK);
	VERSION_ORIGINAL.SoundFlags.v  = (ConfigDat.SoundFlags.v & SNDF_SE_ENABLE);
	VERSION_ORIGINAL.SoundFlags.v |= (BGM_Enabled() * SNDF_BGM_ENABLE);
	VERSION_ORIGINAL.CheckSum.v = VERSION_ORIGINAL.CheckSumCalculate();

	ConfigDat.SoundFlags.v = VERSION_ORIGINAL.SoundFlags.v;
	ConfigDat.SoundFlags.v |= (!BGM_GainApply() * SNDF_BGM_NOT_VOL_NORM);

	if(const auto maybe_topleft = WndBackend_Topleft()) {
		const auto& topleft = maybe_topleft.value();
		ConfigDat.WindowLeft.v = topleft.first;
		ConfigDat.WindowTop.v = topleft.second;
	}

	std::apply([](const auto&... version) {
		(..., VersionSave(version));
	}, VERSIONS);

#ifdef PBG_DEBUG
	FileWrite(DBG_FN, std::span(&DebugDat, 1));
#endif
}
