/*                                                                           */
/*   PBGMIDI.c   ＭＩＤＩ管理用関数                                          */
/*                                                                           */
/*                                                                           */

#include "game/midi.h"
#include "game/endian.h"
#include "platform/midi_backend.h"
#include <algorithm>
#include <thread>
#include <assert.h>
#pragma message(PBGWIN_PBGMIDI_H)


#define MID_STDTEMPO	(1<<7)	// 標準のテンポ

using namespace std::chrono_literals;

// MIDI protocol
// -------------

// Valid values for the upper nibble of the MIDI status byte.
enum class MID_EVENT_KIND : uint8_t {
	NOTE_OFF = 0x80,
	NOTE_ON = 0x90,
	NOTE_AFTERTOUCH = 0xA0,
	CONTROLLER = 0xB0,
	PROGRAM_CHANGE = 0xC0,
	CHANNEL_AFTERTOUCH = 0xD0,
	PITCH_BEND = 0xE0,
	SYSEX = 0xF0,
	META = 0xFF,

	FIRST = NOTE_OFF,
};
// -------------


// MIDI protocol
// -------------

static constexpr uint8_t MIDI_CHANNELS = 16;
// -------------

// Standard MIDI File format
// -------------------------
#pragma pack(push, 1)

typedef struct {
	U32BE	MThd;
	U32BE	size;
} SMF_FILE;

typedef struct {
	U16BE	format;
	U16BE	track;
	U16BE	timebase;
} SMF_MAIN;

typedef struct {
	U32BE	MTrk;
	U32BE	size;
} SMF_TRACK;

#pragma pack(pop)
// -------------------------


//// みでぃ用構造体 ////
typedef struct{
	// 以下は外部から変更＆参照しないこと //
	unsigned int	FadeCount;	// フェードＩ／Ｏカウンタ
	char	FadeFlag;	// フェードＩ／Ｏフラグ(In or Out or 無し)
	int	FadeWait;	// フェードＩ／Ｏウェイト

	uint8_t	MaxVolume;	// ボリュームの最大値(メッセージでも変化,0-127)
	uint8_t	NowVolume;	// 現在のボリューム(0-127)
	MID_BACKEND_STATE	state;	// 現在の状態
} MID_DEVICE;



struct MID_TEMPO {
	MID_REALTIME qn_duration;
	uint16_t ppqn;

	MID_REALTIME RealtimeFromDelta(int32_t delta) const {
		// Let's round to average out any truncated nanoseconds...
		const auto qn_times_delta = (qn_duration * delta);
		return ((delta > 0)
			? ((qn_times_delta + MID_REALTIME(ppqn / 2)) / ppqn)
			: ((qn_times_delta - MID_REALTIME(ppqn / 2)) / ppqn)
		);
	}
};

struct MID_EVENT {
	// An exact `enum class` value for easy `switch`ing.
	MID_EVENT_KIND kind;

	// Raw MIDI status byte, including the channel if <0xF0.
	uint8_t status;

	// Meta event code; only valid if ([status] == MID_EVENT_KIND::META).
	uint8_t meta;

	// Extra data after the status byte. *Not* a complete raw MIDI message!
	std::span<const uint8_t> extra_data;

	uint8_t Channel(void) const {
		assert(kind < MID_EVENT_KIND::SYSEX);
		return (status & 0xf);
	}

	// Converts the event to a raw MIDI message and sends that to the output
	// device.
	void Send(void) const;
};

class MID_TRACK_ITERATOR {
	BYTE_BUFFER_CURSOR<const uint8_t> cursor;
	uint8_t status = 0x00;

public:
	MID_TRACK_ITERATOR() = default;
	MID_TRACK_ITERATOR(const MID_TRACK_ITERATOR& other) = default;
	MID_TRACK_ITERATOR(const std::span<const uint8_t> data) :
		cursor(data), status(0x00) {
	}

	// Reads a MIDI variable-length quantity and advances [work] accordingly.
	// Used for both delta times and multi-byte event lengths. Returns -1 if
	// the end of the track was reached in the middle of the parsed value.
	uint32_t ConsumeVLQ(void);

	// Reads the next MIDI event and advances [work] according to its size.
	// Returns std::nullopt if the end of the sequence has been reached.
	std::optional<MID_EVENT> ConsumeEvent(void);
};

struct MID_TRACK {
	std::span<const uint8_t> data;
	MID_TRACK_ITERATOR it;
	MID_PULSE next_pulse = 0;

	// Time until the next MIDI event. The MIDI events add their delta time
	// converted into seconds according to the current tempo, and the play
	// callback subtracts its interval. If ≤0, the next event is processed.
	MID_REALTIME next_time = 0s;

	bool play = true;

	// Reads a MIDI delta time from the iterator and updates [next_time] and
	// [next_pulse] accordingly.
	void ConsumeDelta(const MID_TEMPO& tempo);
};

struct MID_SEQUENCE {
	BYTE_BUFFER_OWNED smf = nullptr;
	std::unique_ptr<MID_TRACK[]> track_buf = nullptr;
	std::span<MID_TRACK> tracks;
	MID_TEMPO tempo = { .qn_duration = 1s /* 60 BPM */ };

	// Applies the event to the sequence state and consumes the following delta
	// time on the given track.
	void Process(MID_TRACK& track, const MID_EVENT& event);

	void Rewind(void);
};


// ローカルな関数 //
static void Mid_GMReset(void);

// さらに？ローカルな関数 //
static void MidFadeIOFunc(void);

// グローバル＆名前空間でローカルな変数 //
MID_DEVICE	Mid_Dev;
static MID_SEQUENCE Mid_Seq;
uint8_t Mid_PlayTable[16][128];	// スペアナ用
uint8_t Mid_PlayTable2[16][128];	// レベルメーター用
uint8_t Mid_NoteTable[16][128];	// ノート表示用
uint8_t Mid_NoteWTable[16][128];	// ノート表示用(2)
uint8_t Mid_PanpodTable[16];	// パンポット
uint8_t Mid_ExpressionTable[16];	// エクスプレッション
uint8_t Mid_VolumeTable[16];	// ボリューム

static uint8_t Mid_MulTempo = MID_STDTEMPO;
std::chrono::duration<int32_t, std::milli> Mid_PlayTime = 0s;


bool Mid_Start(void)
{
	return MidBackend_Init();
}

void Mid_End(void)
{
	Mid_Stop();
	MidBackend_Cleanup();
}

void Mid_Play(void)
{
	if(
		!MidBackend_DeviceName() ||
		Mid_Seq.tracks.empty() ||
		(Mid_Dev.state == MID_BACKEND_STATE::PLAY)
	) {
		return;
	}

	Mid_Dev.FadeFlag  = 0;
	Mid_Dev.MaxVolume = 127;
	Mid_Dev.NowVolume = 127;
	Mid_Volume(Mid_Dev.NowVolume);

	Mid_Seq.Rewind();
	for(auto& t : Mid_Seq.tracks) {
		t.next_time = 0s;
	}

	Mid_GMReset();
	MidBackend_StartTimer();
	Mid_Dev.state = MID_BACKEND_STATE::PLAY;
}

void Mid_Stop(void)
{
	if(Mid_Dev.state == MID_BACKEND_STATE::STOP) {
		return;
	}

	Mid_PlayTime     = 0s;
	Mid_Dev.FadeFlag = 0;

	MidBackend_StopTimer();
	MidBackend_Panic();

	Mid_TableInit();
	for(auto i = 0; i < MIDI_CHANNELS; i++) {
		MidBackend_Out((0xb0 + i), 0x7b, 0x00);	// オール・ノート・オフ
		MidBackend_Out((0xb0 + i), 0x78, 0x00);	// オール・サウンド・オフ
	}

	Mid_Dev.state = MID_BACKEND_STATE::STOP;
}

// 各種テーブルの初期化 //
void Mid_TableInit(void)
{
	for(auto i = 0; i < MIDI_CHANNELS; i++) {
		for(auto j = 0; j < 128; j++) {
			Mid_PlayTable[i][j]  = 0;
			Mid_PlayTable2[i][j] = 0;
			Mid_NoteTable[i][j]  = 0;
			Mid_NoteWTable[i][j] = 0;
		}

		Mid_PanpodTable[i]     = 0x40;
		Mid_ExpressionTable[i] = 0x7f;
		Mid_VolumeTable[i]     = 0x64;
	}
}

void Mid_Volume(uint8_t volume)
{
	// マスター・ボリューム : F0 7F 7F 04 01 VolumeLowByte VolumeHighByte F7   //
	// 下位バイトは SC-88ST Pro では 00 として扱われるらしい(取扱説明書より) //

	uint8_t msg[8] = { 0xf0, 0x7f, 0x7f, 0x04, 0x01, 0x00, volume, 0xf7 };
	MidBackend_Out(msg);

	// これより下は削った方が良いかも //
	//temp.w.d1 = temp.w.d2 = volume;
	//midiOutSetVolume(Mid_Dev.mp,temp.dd);
}

void Mid_Tempo(char tempo)
{
	Mid_MulTempo = MID_STDTEMPO + tempo;
}

void Mid_FadeOut(uint8_t speed)
{
	Mid_Dev.FadeFlag  = -1;
	Mid_Dev.FadeCount = 0;

	// MaxVolume,FadeWait に 1 だけ加算しているのは、０除算防止のため //
	Mid_Dev.FadeWait  = ((256-speed)*4)/(Mid_Dev.MaxVolume+1) + 1;
}

void Mid_GMReset(void)
{
	// GM SystemOn : F0H 7EH 7FH 09H 01H F7H //
	uint8_t msg[6] = { 0xf0, 0x7e, 0x7f, 0x09, 0x01, 0xf7 };
	MidBackend_Out(msg);

	// ここで50ms以上待つこと! //
	std::this_thread::sleep_for(50ms);
}

bool Mid_ChgDev(char pos)
{
	// 各関数に合わせて停止処理を行う //
	Mid_Stop();

	const auto ret = MidBackend_DeviceChange(pos);
	if(ret) {
		Mid_Play();
	}
	return ret;
}

bool Mid_Load(BYTE_BUFFER_OWNED buffer)
{
	Mid_Seq = {};
	Mid_Seq.smf = std::move(buffer);

	auto cursor = Mid_Seq.smf.cursor();
	const auto maybe_midhead = cursor.next<SMF_FILE>();
	if(!maybe_midhead) {
		return false;
	}
	const auto& midhead = maybe_midhead.value()[0];

	if(midhead.MThd != 0x4D546864) { // "MThd"
		return false;
	}

	const auto maybe_midmain = cursor.next<SMF_MAIN>();
	if(!maybe_midmain) {
		return false;
	}
	const auto midmain = maybe_midmain.value()[0];

	Mid_Seq.tempo.ppqn = midmain.timebase;

	Mid_Seq.track_buf = std::unique_ptr<MID_TRACK[]>(
		new (std::nothrow) MID_TRACK[midmain.track]
	);
	if(!Mid_Seq.track_buf) {
		return false;
	}
	Mid_Seq.tracks = { Mid_Seq.track_buf.get(), midmain.track };

	for(auto& track : Mid_Seq.tracks) {
		const auto maybe_midtrack = cursor.next<SMF_TRACK>();
		if(!maybe_midtrack) {
			return false;
		}
		const auto midtrack = maybe_midtrack.value()[0];

		const auto maybe_data = cursor.next<uint8_t>(midtrack.size);
		if(!maybe_data) {
			return false;
		}
		track.data = maybe_data.value();
	}

	Mid_Seq.Rewind();
	return true;
}

void MID_SEQUENCE::Rewind(void)
{
	//Mid_Fade = 0;
	//Mid_MulTempo = MID_STDTEMPO;

	Mid_PlayTime = 0s;
	for(auto& t : Mid_Seq.tracks) {
		// We do *not* reset [next_time] here. This preserves the overshot
		// amount of time when we rewind at the end of the track.
		t.it = { t.data };
		t.play = true;
		t.next_pulse = 0;
		t.ConsumeDelta(tempo);
	}
}

uint32_t MID_TRACK_ITERATOR::ConsumeVLQ(void)
{
	uint8_t temp = 0;
	uint32_t ret = 0;
	do {
		const auto maybe_temp = cursor.next<uint8_t>(1);
		if(!maybe_temp) {
			return (std::numeric_limits<uint32_t>::max)();
		}
		temp = maybe_temp.value()[0];
		ret = ((ret << 7) | (temp & 0x7f));
	} while(temp & 0x80);
	return ret;
}

void MID_TRACK::ConsumeDelta(const MID_TEMPO& time)
{
	const auto delta = it.ConsumeVLQ();
	if(delta == -1) {
		play = false;
		return;
	}
	next_pulse += delta;
	next_time += time.RealtimeFromDelta(delta);
}

std::optional<MID_EVENT> MID_TRACK_ITERATOR::ConsumeEvent(void)
{
	const auto maybe_status = cursor.next<uint8_t>(1);
	if(!maybe_status) {
		return std::nullopt;
	}
	if(maybe_status.value()[0] >= std::to_underlying(MID_EVENT_KIND::FIRST)) {
		status = maybe_status.value()[0];
	} else {
		cursor.cursor--;
	}
	assert(status >= std::to_underlying(MID_EVENT_KIND::FIRST));
	const auto kind = ((status > std::to_underlying(MID_EVENT_KIND::SYSEX))
		? MID_EVENT_KIND::META
		: static_cast<MID_EVENT_KIND>(status & 0xf0)
	);

	std::optional<std::span<const uint8_t>> extra_data = std::nullopt;
	uint8_t meta = 0;
	switch(kind) {
	case MID_EVENT_KIND::NOTE_OFF:
	case MID_EVENT_KIND::NOTE_ON:
	case MID_EVENT_KIND::NOTE_AFTERTOUCH:
	case MID_EVENT_KIND::CONTROLLER:
	case MID_EVENT_KIND::PITCH_BEND:
		extra_data = cursor.next<uint8_t>(2);
		break;
	case MID_EVENT_KIND::PROGRAM_CHANGE:
	case MID_EVENT_KIND::CHANNEL_AFTERTOUCH:
		extra_data = cursor.next<uint8_t>(1);
		break;
	case MID_EVENT_KIND::SYSEX:
		extra_data = cursor.next<uint8_t>(ConsumeVLQ());
		break;
	case MID_EVENT_KIND::META:
		meta = cursor.next<uint8_t>(1)
			.transform([](auto v) { return v[0]; })
			.value_or(0);
		extra_data = cursor.next<uint8_t>(ConsumeVLQ());
		break;
	default:
		assert(!"Unimplemented MIDI system message");
		break;
	}
	if(!extra_data) {
		return std::nullopt;
	}
	return MID_EVENT{ kind, status, meta, extra_data.value() };
}


static void MidFadeIOFunc(void)
{
	if(Mid_Dev.FadeFlag==0) return;

	if(Mid_Dev.FadeCount % Mid_Dev.FadeWait == 0){
		Mid_Dev.NowVolume += Mid_Dev.FadeFlag;
		for(int track = 0; track < 16; track++) {
			const uint8_t volume = (
				(Mid_VolumeTable[track] * Mid_Dev.NowVolume) /
				(Mid_Dev.MaxVolume + 1)
			);
			MidBackend_Out((0xb0 + track), 0x07, volume);
		}
		//Mid_Volume(Mid_Dev.NowVolume);
		if(Mid_Dev.NowVolume==0 || Mid_Dev.NowVolume==Mid_Dev.MaxVolume){
			Mid_Dev.FadeFlag = 0;
			Mid_Stop();
		}
	}

	Mid_Dev.FadeCount++;
}

void Mid_Proc(MID_REALTIME delta)
{
	const auto interval = ((delta * Mid_MulTempo) / MID_STDTEMPO);

	// Advance the timer of all tracks first. Must be done before we process
	// anything because tempo events on other tracks will need to resynchronize
	// the timer.
	for(auto& p : Mid_Seq.tracks) {
		if(p.play) {
			p.next_time -= interval;
		}
	}

	// Process all events.
	for(auto& p : Mid_Seq.tracks) {
		while(p.play && (p.next_time <= 0s)) {
			const auto maybe_event = p.it.ConsumeEvent();
			if(!maybe_event) {
				p.play = false;
				continue;
			}
			const auto& event = maybe_event.value();
			Mid_Seq.Process(p, event);
			event.Send();
		}
	}

	// Check if we're done. (We might have processed the final End of Track
	// event in the loop above.)
	const bool still_playing = std::ranges::any_of(
		Mid_Seq.tracks, &MID_TRACK::play
	);

	Mid_PlayTime += std::chrono::round<decltype(Mid_PlayTime)>(delta);

	MidFadeIOFunc();

	if(!still_playing) {
		Mid_Seq.Rewind();
	}
}

void MID_EVENT::Send(void) const
{
	switch(kind) {
	case MID_EVENT_KIND::SYSEX: { // エクスクルーシブ
		auto* msg = static_cast<uint8_t *>(_malloca(extra_data.size() + 1));
		if(!msg) {
			break;
		}
		msg[0] = 0xf0;
		std::ranges::copy(extra_data, (msg + 1));
		MidBackend_Out({ msg, (extra_data.size() + 1) });
		_freea(msg);
		break;
	}

	// ３バイト： コントロールチェンジ or 発音 or 変更 or ノートオフ
	case MID_EVENT_KIND::CONTROLLER:
	case MID_EVENT_KIND::NOTE_ON:
	case MID_EVENT_KIND::NOTE_AFTERTOUCH:
	case MID_EVENT_KIND::NOTE_OFF:
	case MID_EVENT_KIND::PITCH_BEND:
		MidBackend_Out(status, extra_data[0], extra_data[1]);
		break;

	// ２バイト
	case MID_EVENT_KIND::PROGRAM_CHANGE:
	case MID_EVENT_KIND::CHANNEL_AFTERTOUCH:
		MidBackend_Out(status, extra_data[0]);
		break;
	}
}

void MID_SEQUENCE::Process(MID_TRACK& track, const MID_EVENT& event)
{
	switch(event.kind) {
	case MID_EVENT_KIND::META: // 制御用データ(出力のないものだけ出力)
		switch(event.meta) {
		case 0x2f: // トラック終了
			track.play = false;
			return;

		case 0x51: { // テンポ
			uint32_t tempo_new = 0;
			for(const auto byte : event.extra_data) {
				tempo_new = ((tempo_new << 8) + byte);
			}
			tempo.qn_duration = std::chrono::duration_cast<MID_REALTIME>(
				std::chrono::microseconds{ tempo_new }
			);

			// Recalculate the next tick on all tracks, preserving the amount
			// of time we've overshot when reaching this one.
			for(auto& other : tracks) {
				// Note that this is also correct in case [other] still has to
				// process multiple events before [track.next_pulse]. In this
				// case, all of these events *must* be processed now, which we
				// ensure by calculating the new [next_time] at the current
				// tempo.
				const auto delta = (other.next_pulse - track.next_pulse);
				other.next_time = (
					track.next_time + tempo.RealtimeFromDelta(delta)
				);
			}
			break;
		}

		default:
			break;
		}

		// ここに謎の一行があります //
		break;

	case MID_EVENT_KIND::CONTROLLER: // コントロールチェンジ
		switch(event.extra_data[0]) {
		case 0x07: // ボリューム
			Mid_VolumeTable[event.Channel()] = event.extra_data[1];
			break;
		case 0x0a: // パンポット
			Mid_PanpodTable[event.Channel()] = event.extra_data[1];
			break;
		case 0x0b: // エクスプレッション
			Mid_ExpressionTable[event.Channel()] = event.extra_data[1];
			break;
		default:
			break;
		}
		break;

	case MID_EVENT_KIND::NOTE_OFF: // ノートオフ
		Mid_NoteTable[event.Channel()][event.extra_data[0]]  = 0;
		break;

	case MID_EVENT_KIND::NOTE_ON:
	case MID_EVENT_KIND::NOTE_AFTERTOUCH: { // ３バイト：発音 or 変更
		const auto channel = event.Channel();
		const auto note = event.extra_data[0];
		const auto vel = event.extra_data[1];

		if(Mid_PlayTable[channel][note] < vel) {
			Mid_PlayTable[channel][note]  = vel;
			Mid_PlayTable2[channel][note] = vel;
		}
		// Mid_PlayTable[channel][note]  += vel;
		// Mid_PlayTable2[channel][note] += vel;
		Mid_NoteTable[channel][note]  = vel;
		if(Mid_NoteTable[channel][note]) {
			Mid_NoteWTable[channel][note] = 5;
		}
		break;
	}

	default:
		break;
	}

	track.ConsumeDelta(tempo);
}

Any::string_view Mid_GetTitle(void)
{
	std::optional<MID_EVENT> maybe_ev;

	const auto extra_data_as_string_view = [](const MID_EVENT& ev) {
		const auto* str = reinterpret_cast<const char *>(ev.extra_data.data());
		return Any::string_view{ str, ev.extra_data.size() };
	};

	// 通常のファイル用 たまに変なファイルだと間違ったものを表示するが... //
	for(const auto& track : Mid_Seq.tracks) {
		MID_TRACK_ITERATOR it = { track.data };
		while((it.ConsumeVLQ() != -1) && (maybe_ev = it.ConsumeEvent())) {
			const auto& ev = maybe_ev.value();
			// Sequence Name
			if((ev.kind == MID_EVENT_KIND::META) && (ev.meta == 0x03)) {
				return extra_data_as_string_view(ev);
			}
		}
	}

	// タイトルのはずなのに別のところに記述しているファイル用 //
	for(const auto& track : Mid_Seq.tracks) {
		MID_TRACK_ITERATOR it = { track.data };
		while((it.ConsumeVLQ() != -1) && (maybe_ev = it.ConsumeEvent())) {
			const auto& ev = maybe_ev.value();
			// Text Event
			if((ev.kind == MID_EVENT_KIND::META) && (ev.meta == 0x01)) {
				return extra_data_as_string_view(ev);
			}
		}
	}

	return {};
}
