/*
 *   Unused MIDI code
 *
 */

// MIDI function flags. These were intended to differentiate between different
// approaches for (1) updating the internal MIDI state and (2) sending MIDI
// data to the playback device. The original game only used [MIDFN_CALLBACK]
// and had no implementation for [MIDFN_STREAM], and this codebase will go a
// different route altogether by decoupling (1) from (2) in order to be more
// portable.
#define MIDFN_NOPLAY	0x00	// 再生しない
#define MIDFN_CALLBACK	0x01	// CALLBACK_FN を使用して再生
#define MIDFN_MIDLOOP	0x02	// ゲーム等の一定スピードのループで再生
#define MIDFN_STREAM	0x04	// MIDI_STREAM を使用して再生(予約のみ)

// Indicates whether playback should loop ([MIDPL_NORM]) or stop ([MIDPL_STOP])
// after the MIDI file reached its end. This distinction is better expressed in
// terms of loop points inside the BGM files themselves; it makes little sense
// to control this through game code, and the original game only used
// [MIDPL_NORM] anyway.
#define MIDPL_NORM		0x00	// 曲の最後にきたら最初に戻って再生
#define MIDPL_STOP		0x01	// 曲の最後にきたら停止

// Why would you ever do this?! It's going to annoy every headphone user, and
// some channels will override this position sooner or later with a CC #10
// inside the MIDI sequence anyway. A 無駄関数 indeed.
void Mid_Pan(char pan);						// 全てのチャンネルのパンを変更する(無駄関数)

// Per-frame function for MIDFN_MIDLOOP. In that mode, the game would call this
// function on every frame.
void Mid_LoopFunc(DWORD time);				// 一定間隔ループを自前で生成して再生するとき使用

// These are internal values that no other code should care about.
DWORDLONG Mid_GetPlaycount1(void);		// playcount1 の取得
DWORDLONG Mid_GetPlaycount2(void);		// playcount2 の取得
DWORDLONG Mid_GetTicks(void);			// tick の取得
void Mid_GetData(MID_DATA *pData);		// 全情報を取得
