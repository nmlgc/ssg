/*
 *   Config data
 *
 */

#pragma once

#include "LEVEL.H"
#include "DirectXUTYs/DD_UTY.H"
#include "platform/buffer.h"

// フラグ //
#define GRPF_MASK				(~0x07)
#define GRPF_ALPHA_ENABLE		0x01		// 半透明が有効(無効時はメッシュ???)
#define GRPF_WINDOW_UPPER		0x02		// メッセージウィンドウを上の方に表示する
#define GRPF_MSG_DISABLE		0x04		// 会話シーンを省略

#define SNDF_MASK				(~0x03)
#define SNDF_MIDI_ENABLE		0x01		// ＭＩＤＩが有効
#define SNDF_WAVE_ENABLE		0x02		// ＷＡＶＥが有効

#define INPF_MASK				(~0x07)
#define INPF_JOYPAD_ENABLE		0x01		// パッドが有効
#define INPF_Z_MSKIP_ENABLE		0x02		// Ｚキーでメッセージを送れる
#define INPF_Z_SPDDOWN_ENABLE	0x04		// 押しっぱなしでシフト移動

#define DBGF_MASK				(~0x03)
#define DBGF_DISPLAYINFO		0x01		// デバッグ情報を出力する
#define DBGF_HIT_ON				0x02		// 当たり判定が有効

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

// Option class //
template <typename T> struct CONFIG_OPTION_VALUE {
	T v;

	const size_t Size() const {
		return sizeof(T);
	}

	bool Read(BYTE_BUFFER_CURSOR<const uint8_t>& cursor) {
		const auto maybe_loaded = cursor.next<T>();
		if(!maybe_loaded) {
			return false;
		}
		const auto loaded = maybe_loaded.value()[0];
		v = loaded;
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
	template <typename T> using OPTION = CONFIG_OPTION_VALUE<T>;

	// 難易度関連 //
	OPTION<uint8_t> GameLevel = { GAME_NORMAL };	// 難易度
	OPTION<uint8_t> PlayerStock = { 2 };	// 初期メイド数？
	OPTION<uint8_t> BombStock = { 2 };	// 初期ボム数

	// グラフィック関連 //
	OPTION<uint8_t> DeviceID = { 0 };	// 何番目のデバイスか
	OPTION<BITDEPTH> BitDepth;	// ビット深度

	// Target frame rate = 60 / [FPSDivisor]. 0 disables any frame rate
	// limitation.
	OPTION<uint8_t> FPSDivisor = { 1 }; // 60Fps Mode

	OPTION<uint8_t> GraphFlags = { 0 };	// グラフィックに関するフラグ

	// サウンド・ＭＩＤＩ関連 //
	OPTION<uint8_t> SoundFlags = { (SNDF_WAVE_ENABLE | SNDF_MIDI_ENABLE) };

	// 入力に関するフラグ
	OPTION<uint8_t> InputFlags = { INPF_Z_MSKIP_ENABLE };

	OPTION<uint8_t> DebugFlags = { 0 };	// デバッグに関するフラグ

	OPTION<uint8_t> PadTama = { 1 };
	OPTION<uint8_t> PadBomb = { 2 };
	OPTION<uint8_t> PadShift = { 0 };
	OPTION<uint8_t> PadCancel = { 0 };

	// エキストラステージ判定用フラグ
	OPTION<uint8_t> ExtraStgFlags = { 0 };

	// ワケ有りでここにいるのです(チェックサムの範囲外)
	OPTION<uint8_t> StageSelect = { 0 };
} CONFIG_DATA;

// Active configuration
extern CONFIG_DATA ConfigDat;

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
