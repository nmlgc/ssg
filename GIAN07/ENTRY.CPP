/*
 *   Generic, cross-platform subsystem initialization and cleanup
 *
 */

#include "GIAN07/ENTRY.H"
#include "GIAN07/CONFIG.H"
#include "GIAN07/GAMEMAIN.H"
#include "GIAN07/LOADER.H"
#include "platform/graphics_backend.h"
#include "platform/input.h"
#include "platform/path.h"
#include "platform/text_backend.h"
#include "platform/window_backend.h"
#include "game/bgm.h"
#include "game/debug.h"
#include "game/frame.h"
#include "game/input.h"
#include "game/snd.h"
#include "obj/platform_constants.h"

// Screenshots
// -----------

const uint8_t& Grp_ScreenshotEffort = ConfigDat.ScreenshotEffort.v;
// -----------

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

// Pad bindings
// ------------

static constexpr std::array<INPUT_PAD_BINDING, 4> PadBindings = { {
	{ ConfigDat.PadTama.v, KEY_TAMA },
	{ ConfigDat.PadBomb.v, KEY_BOMB },
	{ ConfigDat.PadShift.v, KEY_SHIFT },
	{ ConfigDat.PadCancel.v, KEY_ESC },
} };
std::span<const INPUT_PAD_BINDING> Key_PadBindings = PadBindings;
// ------------

bool XInit(void)
{
	std::filesystem::current_path(PathForData());

#ifdef WIN32
	// User data and game data directories are one and the same on Windows,
	// and we already ship with the skeleton in place.
#elif !defined(PATH_SKELETON)
	#pragma message("No user data skeleton directory defined. This does not matter for development builds, but package builds should set the environment variable `PATH_SKELETON` accordingly.")
#else
	using copy_opts = std::filesystem::copy_options;

	auto path_data = PathForData();
	std::error_code ec;
	const auto opts = (copy_opts::skip_existing | copy_opts::recursive);
	std::filesystem::copy(PATH_SKELETON, PathForData(), opts, ec);
#endif

	DebugSetup();

	// コンフィグをロードする //
	ConfigLoad();
	Grp_FPSDivisor = ConfigDat.FPSDivisor.v;
	ConfigDat.MidFlags.v = Mid_SetFlags(ConfigDat.MidFlags.v);

	// コンフィグ依存の初期化処理
	if(!GrpBackend_Enum()) {
		return false;
	}

	// グラフィックの初期化 //
	const auto maybe_params = Grp_InitOrFallback(ConfigDat.GraphicsParams());
	if(!maybe_params) {
		return false;
	}
	ConfigDat.GraphicsParamsApply(maybe_params.value().live);
	GrpBackend_SetClip(GRP_RES_RECT);

	// キーボード(JoyPad)入力を受け付ける //
	Key_Start();

	// ＢＧＭの初期化 //
	if(ConfigDat.SoundFlags.v & SNDF_BGM_ENABLE) {
		BGM_Init();
	}
	if(!BGM_PackSet(ConfigDat.BGMPack.v)) {
		ConfigDat.BGMPack.v.clear();
	}
	BGM_SetGainApply(!(ConfigDat.SoundFlags.v & SNDF_BGM_NOT_VOL_NORM));
	Grp_ScreenshotSetPrefix(u8"screenshots/");
	LoaderInit();
	return true;
}

void XCleanup(void)
{
	LoaderCleanup();
	ConfigSave();
	TextBackend_Cleanup();
	GrpBackend_Cleanup();
	BGM_Cleanup();
	Snd_Cleanup();
	Key_End();
}

void XGrpTry(const GRAPHICS_PARAMS& prev, GRAPHICS_PARAMS& params)
{
	if(prev == params) {
		return;
	}
	if(const auto maybe_topleft = WndBackend_Topleft()) {
		const auto& topleft = maybe_topleft.value();
		params.left = topleft.first;
		params.top = topleft.second;
	}
	auto maybe_result = Grp_Init(prev, params);
	if(!maybe_result) {
		// Try resetting to the previous configuration, or, if necessary,
		// attempt anything to get graphics back working again.
		maybe_result = Grp_InitOrFallback(prev);
	}
	if(maybe_result) {
		const auto& result = maybe_result.value();
		TextObj.WipeBeforeNextRender();
		ConfigDat.GraphicsParamsApply(result.live);
		if(result.reload_surfaces) {
			ReloadGraph();
		}

		// TODO: Assumes that 8-bit mode only ever calls this function within
		// the main menu. If we ever add hotkeys to toggle between 8-bit and
		// 16-bit modes (https://github.com/nmlgc/ssg/issues/72), this has to
		// be solved more cleanly as part of the backend.
		GrpSurface_PaletteApplyToBackend(SURFACE_ID::TITLE);
	}
}

void XGrpTryCycleScale(int_fast8_t delta, bool include_max)
{
	XGrpTry([&](auto& params) {
		const auto fs = params.FullscreenFlags();
		if(fs.fullscreen && !fs.exclusive) {
			using FIT = GRAPHICS_FULLSCREEN_FIT;
			constexpr auto max = std::to_underlying(FIT::COUNT);
			const auto fit = ((std::to_underlying(fs.fit) + max + delta) % max);
			params.SetFlag(GRAPHICS_PARAM_FLAGS::FULLSCREEN_FIT, fit);
		} else if(!fs.fullscreen) {
			const auto max = (Grp_WindowScale4xMax() + include_max);
			params.window_scale_4x = (
				(params.window_scale_4x + max + delta) % max
			);
		}
	});
}

void XGrpTryCycleDisp(void)
{
	XGrpTry([](auto& params) {
		params.flags ^= GRAPHICS_PARAM_FLAGS::FULLSCREEN;
	});
}

void XGrpTryCycleScMode(void)
{
	XGrpTry([](auto& params) {
		params.flags ^= GRAPHICS_PARAM_FLAGS::SCALE_GEOMETRY;
	});
}

bool GameFrame(void)
{
#ifdef SUPPORT_GRP_WINDOWED
	if(SystemKey_Data & SYSKEY_GRP_FULLSCREEN) {
		XGrpTryCycleDisp();
	}
#endif
#ifdef SUPPORT_GRP_SCALING
	if(SystemKey_Data & SYSKEY_GRP_SCALE_UP) {
		XGrpTryCycleScale(+1, false);
	}
	if(SystemKey_Data & SYSKEY_GRP_SCALE_DOWN) {
		XGrpTryCycleScale(-1, false);
	}
	if(SystemKey_Data & SYSKEY_GRP_SCALE_MODE) {
		XGrpTryCycleScMode();
	}
#endif
	if(SystemKey_Data & SYSKEY_GRP_TURBO) {
		static decltype(Grp_FPSDivisor) fps_divisor_prev = (
			(Grp_FPSDivisor != 0) ? Grp_FPSDivisor : 1
		);
		if(Grp_FPSDivisor != 0) {
			fps_divisor_prev = Grp_FPSDivisor;
			ConfigDat.FPSDivisor.v = Grp_FPSDivisor = 0;
		} else {
			ConfigDat.FPSDivisor.v = Grp_FPSDivisor = fps_divisor_prev;
		}
	}
#ifdef SUPPORT_GRP_API
	if(SystemKey_Data & SYSKEY_GRP_API) {
		XGrpTry([](auto& params) {
			params.api = ((params.api + 1) % GrpBackend_APICount());
		});
	}
#endif

	// Strictly superior to waiting [CWIN_KEYWAIT] frames: We won't get a key
	// release scancode if we recreate the window on a switch into exclusive
	// fullscreen, and we save a dependency on `WindowSys.h`.
	SystemKey_Data &= ~(
		SYSKEY_GRP_FULLSCREEN |
		SYSKEY_GRP_SCALE_UP |
		SYSKEY_GRP_SCALE_DOWN |
		SYSKEY_GRP_SCALE_MODE |
		SYSKEY_GRP_TURBO |
		SYSKEY_GRP_API
	);

	bool quit = false;
	GameMain(quit);
	return !quit;
}
