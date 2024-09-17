/*                                                                           */
/*   PBGMIDI.h   ＭＩＤＩ管理用関数                                          */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_PBGMIDI_H
#define PBGWIN_PBGMIDI_H	"PBGMIDI : Version 0.31 : Update 2000/08/04"

// 更新履歴 //
// 2000/08/04 : Mid_Free() をグローバルにした。

// 2000/03/22 : MIDI フェードアウト関数をマスターボリュームからＣＣのボリュームに変更
//            : 処理の追いつかない MIDI があった場合は、メッセージ送出に工夫が必要かも




#include "game/narrow.h"
#include "game/volume.h"
#include "platform/buffer.h"

enum class MID_FLAGS : uint8_t {
	_HAS_BITFLAG_OPERATORS,
	NONE = 0x00,
	FIX_SYSEX_BUGS = 0x01,
	
	MASK = FIX_SYSEX_BUGS,
};

enum class MID_BACKEND_STATE : uint8_t {
	STOP,	// 停止している
	PLAY,	// 再生中
	PAUSE,	// 一時停止
};

// A position within a MIDI sequence. We need negative numbers for proper
// distance calculations.
using MID_PULSE = int64_t;

// Sufficiently precise realtime values for MIDI event timing. Signed 64-bit
// integers can fit the highest possible tempo × delta time value at nanosecond
// precision (0xFF'FF'FF × 0xF'FF'FF'FF × 10³) , with one bit to spare.
// (Nanoseconds only add a factor of 10³ here because tempo values already use
// microseconds.)
using MID_REALTIME = std::chrono::nanoseconds;

struct MID_LOOP {
	MID_PULSE start = 0;
	MID_PULSE end = 0;

	explicit operator bool() const {
		return (start != end);
	}
};

struct MID_PLAYTIME {
	MID_PULSE pulse_of_last_event_processed = 0;
	MID_PULSE pulse_interpolated = 0;

	// Total playback realtime of the current sequence.
	std::chrono::duration<int32_t, std::milli> realtime = (
		std::chrono::duration<int32_t, std::milli>::zero()
	);
	MID_REALTIME realtime_since_last_event = MID_REALTIME::zero();
};

// External dependencies
extern const VOLUME& Mid_Volume;
extern const uint8_t& Mid_TempoNum;
extern const uint8_t& Mid_TempoDenom;

//// 関数 ////

// Returns the new current MIDI flags.
[[nodiscard]] MID_FLAGS Mid_SetFlags(MID_FLAGS flags_new);

// Starts outputting the loaded MIDI to the backend.
void Mid_Play(void);						// 再生する

// Stops backend output and resets the tables.
void Mid_Stop(void);						// 停止する

void Mid_Pause(void);
void Mid_Resume(void);

// Returns the current (not maximum) MIDI master volume.
VOLUME Mid_GetFadeVolume(void);

void Mid_UpdateVolume(void);
void Mid_FadeOut(VOLUME volume_start, std::chrono::milliseconds duration);

bool Mid_Load(BYTE_BUFFER_OWNED buffer);	// Load a MIDI file from a buffer

// Sets a loop point for the currently loaded sequence.
void Mid_SetLoop(const MID_LOOP& loop);

bool Mid_Loaded(void);

Any::string_view Mid_GetTitle(void);	// この曲のお名前は？

// Processes and outputs the next time [delta] of the currently loaded MIDI
// sequence.
void Mid_Proc(MID_REALTIME delta);

void Mid_TableInit(void);					// 各種テーブルの初期化



//// グローバル変数 ////
extern uint8_t	Mid_PlayTable[16][128];
extern uint8_t	Mid_PlayTable2[16][128];	// レベルメーター用
extern uint8_t	Mid_NoteTable[16][128];	// ノート表示用
extern uint8_t	Mid_NoteWTable[16][128];	// ノート表示用(2)
extern uint8_t	Mid_PanpodTable[16];	// パンポット
extern uint8_t	Mid_ExpressionTable[16];	// エクスプレッション
extern uint8_t	Mid_VolumeTable[16];	// ボリューム
extern MID_PLAYTIME Mid_PlayTime;


#endif
