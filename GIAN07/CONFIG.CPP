/*
 *   Config data
 *
 */

#include <SDL3/SDL_iostream.h>

#include "CONFIG.H"
#include "game/bgm.h"
#include "game/defer.h"
#include "game/endian.h"
#include "platform/file.h"
#include "platform/window_backend.h"

///// Constants /////
constexpr auto DBG_FN = u8"秋霜DBG.DAT";

// Option interface. Allows us to arrange values from [ConfigDat] into arrays
// that define multiple versions of config files, and can be trivially looped
// over.
template <typename T> concept CONFIG_OPTION = requires(T& t, SDL_IOStream& f) {
	// Reads the configuration value from the given stream. Returns whether the
	// read was successful.
	{ OptionRead(t, f) } -> std::same_as<bool>;

	// Writes the configuration value to the given stream. Returns whether the
	// write was successful.
	{ OptionWrite(f, t) } -> std::same_as<bool>;
};

// Data types
// ----------

template <typename T> using DISK = std::conditional_t<
	std::is_fundamental_v<T>, ENDIAN_SELECT_BIG<T>, T
>;

template <typename T> bool OptionRead(
	CONFIG_OPTION_VALUE<T>& opt, SDL_IOStream& f
)
{
	DISK<T> d_loaded;
	if(!SDL_MustReadIO(&f, &d_loaded, sizeof(d_loaded))) {
		return false;
	}
	const T loaded = d_loaded;
	opt.loaded = loaded;
	if(opt.Validate(loaded)) {
		opt.v = loaded;
	}
	return true;
}

template <typename T> bool OptionWrite(
	SDL_IOStream& f, const CONFIG_OPTION_VALUE<T>& opt
)
{
	const DISK<T> d_v = opt.v;
	return SDL_MustWriteIO(&f, &d_v, sizeof(T));
}

bool OptionRead(std::u8string& opt, SDL_IOStream& f)
{
	U32BE d_len;
	if(!SDL_MustReadIO(&f, &d_len, sizeof(d_len))) {
		return false;
	}
	const uint32_t len = d_len;
	opt.resize_and_overwrite(len, [&f](auto buf, size_t n) {
		return SDL_ReadIO(&f, buf, n);
	});
	return ((opt.size() * sizeof(std::u8string::value_type)) == len);
}

bool OptionWrite(SDL_IOStream& f, const std::u8string& opt)
{
	if(opt.size() > (std::numeric_limits<uint32_t>::max)()) {
		return false;
	}
	const auto d_size = static_cast<U32BE>(opt.size());
	if(!SDL_MustWriteIO(&f, &d_size, sizeof(d_size))) {
		return false;
	}
	const auto str_size = (opt.size() * sizeof(std::u8string::value_type));
	return SDL_MustWriteIO(&f, opt.data(), str_size);
}

template <class... Options> bool OptionRead(
	const std::tuple<Options&...>& opts, SDL_IOStream& f
)
{
	return std::apply(
		[&](auto&...opt) { return (... && OptionRead(opt, f)); }, opts
	);
}

template <class... Options> bool OptionWrite(
	SDL_IOStream& f, const std::tuple<Options&...>& opts
)
{
	return std::apply(
		[&](const auto&...opt) { return (... && OptionWrite(f, opt)); }, opts
	);
}
// ----------

// On-disk config file versions
// ----------------------------

template <typename T> concept VERSION = requires(T t) {
	{ t.Options } -> CONFIG_OPTION;
};

// Original 秋霜CFG.DAT
const struct VERSION_ORIGINAL {
	static constexpr auto FN = u8"秋霜CFG.DAT";

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

	static constexpr auto Options = std::tie(
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
		PaddingByte
	);

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
	static constexpr auto FN = u8"SSG_V00.CFG";

	static constexpr auto Options = std::tie(
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
		ConfigDat.StageSelect
	);
} VERSION_00;

const struct VERSION_01 {
	static constexpr auto FN = u8"SSG_V01.CFG";

	static constexpr auto Options = std::tie(
		VERSION_00.Options,
		ConfigDat.SEVolume,
		ConfigDat.BGMVolume,
		ConfigDat.BGMPack
	);
} VERSION_01;

const struct VERSION_02 {
	static constexpr auto FN = u8"SSG_V02.CFG";

	// SDL 2 used a numeric ID for its API parameter, which I reused for the
	// initial implementation. However, this would couple the user's selection
	// to the order of SDL's render driver list, and SDL's developers have and
	// will continue to change its order.
	// In one of its better improvements, SDL 3 changed SDL_CreateRenderer()'s
	// driver parameter to a string, nudging developers into this more stable
	// direction.
	static inline CONFIG_OPTION_VALUE<int8_t> GraphicsAPIID = { -1 };

	static constexpr auto Options = std::tie(
		VERSION_01.Options,
		GraphicsAPIID,
		ConfigDat.WindowScale4x,
		ConfigDat.WindowLeft,
		ConfigDat.WindowTop,
		ConfigDat.MidFlags,
		ConfigDat.ScreenshotEffort
	);
} VERSION_02;

const struct VERSION_03 {
	static constexpr auto FN = u8"SSG_V03.CFG";

	static constexpr auto Options = std::tie(
		VERSION_01.Options,
		ConfigDat.MidFlags,
		ConfigDat.GraphicsAPI,
		ConfigDat.WindowScale4x,
		ConfigDat.WindowLeft,
		ConfigDat.WindowTop,
		ConfigDat.ScreenshotEffort
	);
} VERSION_03;


// Must be sorted from the newest to the oldest version.
const auto VERSIONS = std::make_tuple(
	VERSION_03,
	VERSION_02,
	VERSION_01,
	VERSION_00,
	VERSION_ORIGINAL
);

bool VersionLoad(const VERSION auto& version)
{
	SDL_IOStream *f = SDL_IOFromFile(version.FN, "rb");
	if(!f) {
		return false;
	}
	defer(SDL_CloseIO(f));
	return OptionRead(version.Options, *f);
}

bool VersionSave(const VERSION auto& version)
{
	SDL_IOStream *f = SDL_IOFromFile(version.FN, "wb");
	if(!f) {
		return false;
	}
	defer(SDL_CloseIO(f));
	return OptionWrite(*f, version.Options);
}
// ----------------------------

GRAPHICS_PARAMS CONFIG_DATA::GraphicsParams(void) const
{
	const auto flags_shifted = (GraphFlags.v >> GRPF_PARAM_SHIFT);
	return {
		.flags = static_cast<GRAPHICS_PARAM_FLAGS>(flags_shifted),
		.device_id = DeviceID.v,
#ifdef SUPPORT_GRP_API
		.api = GrpBackend_APIID(GraphicsAPI),
#endif
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
#ifdef SUPPORT_GRP_API
	GraphicsAPI = GrpBackend_APIString(params.api);
#endif
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
	auto *f = SDL_IOFromFile(DBG_FN, "rb");
	if(!f) {
		return;
	}
	if(!SDL_MustReadIO(f, &DebugDat, sizeof(DebugDat))) {
		DebugDat.Hit        = true;
		DebugDat.MsgDisplay = true;
		DebugDat.DemoSave   = false;
		DebugDat.StgSelect  = 1;
	}
	SDL_CloseIO(f);
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
#ifdef SUPPORT_GRP_API
	if(VERSION_02.GraphicsAPIID.loaded) {
		ConfigDat.GraphicsAPI = GrpBackend_APIString(
			VERSION_02.GraphicsAPIID.v
		);
	}
#endif
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
	SDL_SaveFile(DBG_FN, &DebugDat, sizeof(DebugDat));
#endif
}
