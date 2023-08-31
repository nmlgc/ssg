/*
 *   Config data
 *
 */

#pragma once

#include "LEVEL.H"
#include "DirectXUTYs/DD_UTY.H"
#include "game/input.h"
#include "platform/buffer.h"

// フラグ //
constexpr uint8_t GRPF_MASK	= (~0x07);
constexpr uint8_t GRPF_ALPHA_ENABLE	= 0x01;	// 半透明が有効(無効時はメッシュ???)
constexpr uint8_t GRPF_WINDOW_UPPER	= 0x02;	// メッセージウィンドウを上の方に表示する
constexpr uint8_t GRPF_MSG_DISABLE	= 0x04;	// 会話シーンを省略

constexpr uint8_t SNDF_MASK	= (~0x03);
constexpr uint8_t SNDF_MIDI_ENABLE	= 0x01;	// ＭＩＤＩが有効
constexpr uint8_t SNDF_WAVE_ENABLE	= 0x02;	// ＷＡＶＥが有効

constexpr uint8_t INPF_MASK	= (~0x07);
constexpr uint8_t INPF_JOYPAD_ENABLE	= 0x01;	// パッドが有効
constexpr uint8_t INPF_Z_MSKIP_ENABLE	= 0x02;	// Ｚキーでメッセージを送れる
constexpr uint8_t INPF_Z_SPDDOWN_ENABLE	= 0x04;	// 押しっぱなしでシフト移動

constexpr uint8_t DBGF_MASK	= (~0x03);
constexpr uint8_t DBGF_DISPLAYINFO	= 0x01;	// デバッグ情報を出力する
constexpr uint8_t DBGF_HIT_ON	= 0x02;	// 当たり判定が有効

// Limits //
constexpr const auto STOCK_PLAYER_MAX = 4;
constexpr const auto STOCK_BOMB_MAX = 2;
constexpr const auto FPS_DIVISOR_MAX = 3;
constexpr const auto STAGE_MAX = 6; // ステージ数


// Option interface. Allows us to arrange values from [ConfigDat] into arrays
// that define multiple versions of config files, and can be trivially looped
// over.
template <typename T> concept CONFIG_OPTION = requires(
	T t,
	BYTE_BUFFER_CURSOR<const uint8_t>& cursor_read,
	BYTE_BUFFER_CURSOR<uint8_t>& cursor_write
) {
	// Returns the size of the option value.
	{ t.Size() } -> std::same_as<size_t>;

	// Reads the configuration value from the given [cursor], advancing it by
	// the value of Size(). Returns whether there were enough bytes left and
	// reading should continue.
	{ t.Read(cursor_read) } -> std::same_as<bool>;

	// Writes the configuration value to the given [cursor], advancing it by
	// the value of Size(). Returns whether there were enough bytes left and
	// writing should continue.
	{ t.Write(cursor_write) } -> std::same_as<bool>;
};

constexpr auto ValidateAlways = [](auto v) { return true; };

// Option class, with optional validation of the value against a supported
// static range.
template <
	typename T, bool (*Validate)(T) = ValidateAlways
> struct CONFIG_OPTION_VALUE {
	T v;

	// ([loaded] == std::nullopt): We loaded a config file from an earlier
	// build, where this value was not present yet. It was initialized to its
	// default value.
	// ([loaded] == [v]): Option was present in a config file, and its value
	// passed validation.
	// ([loaded] != [v]): The config file contained an invalid value, and was
	// reset to its default.
	std::optional<T> loaded = std::nullopt;

	const size_t Size() const {
		return sizeof(T);
	}

	bool Read(BYTE_BUFFER_CURSOR<const uint8_t>& cursor) {
		const auto maybe_loaded = cursor.next<T>();
		if(!maybe_loaded) {
			return false;
		}
		loaded = maybe_loaded.value()[0];
		if(Validate(maybe_loaded.value()[0])) {
			v = maybe_loaded.value()[0];
		}
		return true;
	}

	bool Write(BYTE_BUFFER_CURSOR<uint8_t>& cursor) {
		auto maybe_v = cursor.next<T>();
		if(!maybe_v) {
			return false;
		}
		maybe_v.value()[0] = v;
		return true;
	}
};

// オプション保持用構造体 //
typedef struct tagCONFIG_DATA{
	template <
		typename T, bool (*Validate)(T) = ValidateAlways
	> using OPTION = CONFIG_OPTION_VALUE<T, Validate>;

	template <typename T, T Max> using OPTION_BELOW = OPTION<T, [](auto v) {
		return (v <= Max);
	}>;

	template <uint8_t Max> using OPTION_U8_BELOW = OPTION_BELOW<uint8_t, Max>;

	template <uint8_t Mask> using OPTION_FLAGS = OPTION<uint8_t, [](auto v) {
		return !(v & Mask);
	}>;

	// 32 is the WinMM joy button limit //
	using OPTION_WINMM_PAD = OPTION_BELOW<INPUT_PAD_BUTTON, 32>;

	// 難易度関連 //
	// ---------- //

	OPTION_U8_BELOW<GAME_LUNATIC> GameLevel = { GAME_NORMAL };	// 難易度

	// 初期メイド数？
	// Quirk: Off by 2?
	OPTION_U8_BELOW<STOCK_PLAYER_MAX + 2> PlayerStock = { 2 };

	// Quirk: Off by 1?
	OPTION_U8_BELOW<STOCK_BOMB_MAX + 1> BombStock = { 2 }; // 初期ボム数
	// ---------- //

	// グラフィック関連 //
	OPTION<uint8_t> DeviceID = { 0 };	// 何番目のデバイスか
	OPTION<BITDEPTH> BitDepth;	// ビット深度

	// Target frame rate = 60 / [FPSDivisor]. 0 disables any frame rate
	// limitation.
	OPTION_U8_BELOW<FPS_DIVISOR_MAX> FPSDivisor = { 1 }; // 60Fps Mode

	// グラフィックに関するフラグ
	OPTION_FLAGS<GRPF_MASK> GraphFlags = { 0 };

	// サウンド・ＭＩＤＩ関連 //
	OPTION_FLAGS<SNDF_MASK> SoundFlags = {
		(SNDF_WAVE_ENABLE | SNDF_MIDI_ENABLE)
	};

	// 入力に関するフラグ
	OPTION_FLAGS<INPF_MASK> InputFlags = { INPF_Z_MSKIP_ENABLE };

	// デバッグに関するフラグ
	OPTION_FLAGS<DBGF_MASK> DebugFlags = { 0 };

	OPTION_WINMM_PAD PadTama = { 1 };
	OPTION_WINMM_PAD PadBomb = { 2 };
	OPTION_WINMM_PAD PadShift = { 0 };
	OPTION_WINMM_PAD PadCancel = { 0 };

	// エキストラステージ判定用フラグ
	OPTION<uint8_t> ExtraStgFlags = { 0 };

	// ワケ有りでここにいるのです(チェックサムの範囲外)
	OPTION_U8_BELOW<STAGE_MAX> StageSelect = { 0 };
} CONFIG_DATA;

// Active configuration
extern CONFIG_DATA ConfigDat;

static constexpr std::array<INPUT_PAD_BINDING, 4> PadBindings = { {
	{ ConfigDat.PadTama.v, KEY_TAMA },
	{ ConfigDat.PadBomb.v, KEY_BOMB },
	{ ConfigDat.PadShift.v, KEY_SHIFT },
	{ ConfigDat.PadCancel.v, KEY_ESC },
} };

#ifdef PBG_DEBUG
	// デバッグ情報管理用構造体 //
	typedef struct tagDEBUG_DATA{
		int32_t	MsgDisplay;	// デバッグ情報を出力するか
		int32_t	Hit;	// 当たり判定の有・無
		int32_t	DemoSave;	// デモプレイをセーブするか

		uint8_t	StgSelect;	// ステージセレクト(開始ステージ)
	} DEBUG_DATA;

	extern DEBUG_DATA DebugDat;
#endif

///// [関数] /////

extern void ConfigLoad();

extern void ConfigSave(void);	// コンフィグの内容を保存する
