/*
 *   Config data
 *
 */

#pragma once

#include "DirectXUTYs/DD_UTY.H"

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



// オプション保持用構造体 //
typedef struct tagCONFIG_DATA{
	BYTE	GameLevel;		// 難易度
	BYTE	PlayerStock;	// 初期メイド数？
	BYTE	BombStock;		// 初期ボム数

	BYTE	DeviceID;		// 何番目のデバイスか
	BITDEPTH	BitDepth;		// ビット深度

	// Target frame rate = 60 / [FPSDivisor]. 0 disables any frame rate
	// limitation.
	BYTE	FPSDivisor;

	BYTE	GraphFlags;		// グラフィックに関するフラグ
	BYTE	SoundFlags;		// サウンド・ＭＩＤＩ関連のフラグ
	BYTE	InputFlags;		// 入力に関するフラグ

	BYTE	DebugFlags;		// デバッグに関するフラグ

	BYTE	PadTama;
	BYTE	PadBomb;
	BYTE	PadShift;
	BYTE	PadCancel;

	BYTE	ExtraStgFlags;	// エキストラステージ判定用フラグ
	uint32_t	CheckSum;

	BYTE	StageSelect;	// ワケ有りでここにいるのです(チェックサムの範囲外)

	uint32_t CheckSumCalculate() const;
} CONFIG_DATA;

extern CONFIG_DATA ConfigDat;

#ifdef PBG_DEBUG
	// デバッグ情報管理用構造体 //
	typedef struct tagDEBUG_DATA{
		BOOL	MsgDisplay;		// デバッグ情報を出力するか
		BOOL	Hit;			// 当たり判定の有・無
		BOOL	DemoSave;		// デモプレイをセーブするか

		BYTE	StgSelect;		// ステージセレクト(開始ステージ)
	} DEBUG_DATA;

	extern DEBUG_DATA DebugDat;
#endif

///// [関数] /////

// Bit depths can only be validated if we have a graphics adapter.
extern void ConfigLoad(const std::span<const XDDRAWENUM> dxenum);

extern void ConfigSave(void);	// コンフィグの内容を保存する
