/*                                                                           */
/*   PBGMIDI.c   ＭＩＤＩ管理用関数                                          */
/*                                                                           */
/*                                                                           */

#include "PBGMIDI.H"
#include "game/endian.h"
#include "platform/midi_backend.h"
#include <assert.h>
#include <windows.h>
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
	DWORD	FadeCount;	// フェードＩ／Ｏカウンタ
	char	FadeFlag;	// フェードＩ／Ｏフラグ(In or Out or 無し)
	int	FadeWait;	// フェードＩ／Ｏウェイト

	BYTE	MaxVolume;	// ボリュームの最大値(メッセージでも変化,0-127)
	BYTE	NowVolume;	// 現在のボリューム(0-127)
	MID_BACKEND_STATE	state;	// 現在の状態
} MID_DEVICE;



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
};

struct MID_TRACK {
	std::span<const uint8_t> data;
	const uint8_t* work;
	int	count;
	bool	play;
	BYTE	status;

	// Reads a MIDI variable-length quantity and advances [work] accordingly.
	// Used for both delta times and multi-byte event lengths.
	uint32_t ConsumeVLQ(void);

	// Reads the next MIDI event and advances [work] according to its size.
	MID_EVENT ConsumeEvent(void);
};

struct MID_SEQUENCE {
	BYTE_BUFFER_OWNED smf = nullptr;
	std::unique_ptr<MID_TRACK[]> track_buf = nullptr;
	std::span<MID_TRACK> tracks;
	int	timebase;

	// Length of a quarter note. No need to use the 64 bits of
	// `std::chrono::microseconds` if MIDI just uses 28.
	std::chrono::duration<uint32_t, std::micro> tempo;

	std::chrono::milliseconds playcount1;
	DWORDLONG	playcount2;
};


// ローカルな関数 //
static void Mid_GMReset(void);
static BOOL Mid_Init(void);

// さらに？ローカルな関数 //
static void Mid_Process(MID_TRACK& track, const MID_EVENT& event);
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

static BYTE			Mid_MulTempo = MID_STDTEMPO;
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

	Mid_Init();

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

void Mid_Volume(BYTE volume)
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

void Mid_FadeOut(BYTE speed)
{
	Mid_Dev.FadeFlag  = -1;
	Mid_Dev.FadeCount = 0;

	// MaxVolume,FadeWait に 1 だけ加算しているのは、０除算防止のため //
	Mid_Dev.FadeWait  = ((256-speed)*4)/(Mid_Dev.MaxVolume+1) + 1;
}

void Mid_GMReset(void)
{
	// GM SystemOn : F0H 7EH 7FH 09H 01H F7H //

	DWORD	time;
	uint8_t msg[6] = { 0xf0, 0x7e, 0x7f, 0x09, 0x01, 0xf7 };
	MidBackend_Out(msg);

	// ここで50ms以上待つこと! //
	time = timeGetTime();
	while(timeGetTime()-time<=50);
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

	Mid_Seq.timebase = midmain.timebase;
	Mid_Seq.tempo = 1s; // 60 BPM

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

	Mid_Init();
	return true;
}

static BOOL Mid_Init(void)
{
	//Mid_Fade = 0;
	//Mid_MulTempo = MID_STDTEMPO;
	Mid_PlayTime    = 0s;

	Mid_Seq.playcount1 = 0s;
	Mid_Seq.playcount2 = 0;

	for(auto& t : Mid_Seq.tracks) {
		t.work  = t.data.data();
		t.play  = true;
		t.count = t.ConsumeVLQ();	// 初期ウェイトカウントを読むの
	}

	return TRUE;
}

uint32_t MID_TRACK::ConsumeVLQ(void)
{
	uint8_t temp = 0;
	uint32_t ret = 0;
	do {
		temp = *work;
		work++;
		ret = ((ret << 7) | (temp & 0x7f));
	} while(temp & 0x80);
	return ret;
}

MID_EVENT MID_TRACK::ConsumeEvent(void)
{
	if(*(work) >= std::to_underlying(MID_EVENT_KIND::FIRST)) {
		status = *(work);
		work++;
	}
	assert(status >= std::to_underlying(MID_EVENT_KIND::FIRST));
	const auto kind = ((status > std::to_underlying(MID_EVENT_KIND::SYSEX))
		? MID_EVENT_KIND::META
		: static_cast<MID_EVENT_KIND>(status & 0xf0)
	);

	size_t extra_data_size = 0;
	uint8_t meta = 0;
	switch(kind) {
	case MID_EVENT_KIND::NOTE_OFF:
	case MID_EVENT_KIND::NOTE_ON:
	case MID_EVENT_KIND::NOTE_AFTERTOUCH:
	case MID_EVENT_KIND::CONTROLLER:
	case MID_EVENT_KIND::PITCH_BEND:
		extra_data_size = 2;
		break;
	case MID_EVENT_KIND::PROGRAM_CHANGE:
	case MID_EVENT_KIND::CHANNEL_AFTERTOUCH:
		extra_data_size = 1;
		break;
	case MID_EVENT_KIND::SYSEX:
		extra_data_size = ConsumeVLQ();
		break;
	case MID_EVENT_KIND::META:
		meta = *(work++);
		extra_data_size = ConsumeVLQ();
		break;
	default:
		assert(!"Unimplemented MIDI system message");
		break;
	}
	const MID_EVENT ret = { kind, status, meta, { work, extra_data_size } };
	work += extra_data_size;
	return ret;
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
	BOOL		flag = FALSE;
	const auto delta_ms = std::chrono::round<std::chrono::milliseconds>(delta);
	const auto now = (Mid_Seq.playcount2 + (
		(Mid_Seq.playcount1 * Mid_Seq.timebase) / Mid_Seq.tempo
	));

	for(auto& p : Mid_Seq.tracks) {
		if(p.play) {
			flag = TRUE;
			while(p.play && (p.count <= now)) {
				const auto event = p.ConsumeEvent();
				Mid_Process(p, event);
			}
		}
	}

	Mid_PlayTime += delta_ms;

	Mid_Seq.playcount1 += ((delta_ms * Mid_MulTempo) / MID_STDTEMPO);

	MidFadeIOFunc();

	if(!flag){
		Mid_Init();
	}
}

static void Mid_Process(MID_TRACK& track, const MID_EVENT& event)
{
	switch(event.kind) {
	case MID_EVENT_KIND::SYSEX: { // エクスクルーシブ
		auto* msg = static_cast<uint8_t *>(
			_malloca(event.extra_data.size() + 1)
		);
		if(!msg) {
			break;
		}
		msg[0] = 0xf0;
		std::ranges::copy(event.extra_data, (msg + 1));
		MidBackend_Out({ msg, (event.extra_data.size() + 1) });
		_freea(msg);
		break;
	}

	case MID_EVENT_KIND::META: // 制御用データ(出力のないものだけ出力)
		switch(event.meta) {
		case 0x2f: // トラック終了
			track.play = false;
			return;

		case 0x51: { // テンポ
			Mid_Seq.playcount2 += (
				(Mid_Seq.playcount1 * Mid_Seq.timebase) / Mid_Seq.tempo
			);
			Mid_Seq.playcount1 = 0s;
			uint32_t tempo = 0;
			for(const auto byte : event.extra_data) {
				tempo += ((tempo << 8) + byte);
			}
			Mid_Seq.tempo = decltype(Mid_Seq.tempo){ tempo };
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
		}
		MidBackend_Out(event.status, event.extra_data[0], event.extra_data[1]);
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

		MidBackend_Out(event.status, note, vel);
		break;
	}

	case MID_EVENT_KIND::PITCH_BEND: // ３バイト
		MidBackend_Out(event.status, event.extra_data[0], event.extra_data[1]);
		break;

	case MID_EVENT_KIND::PROGRAM_CHANGE:
	case MID_EVENT_KIND::CHANNEL_AFTERTOUCH: // ２バイト
		MidBackend_Out(event.status, event.extra_data[0]);
		break;
	}

	track.count += track.ConsumeVLQ();
}

const char* Mid_GetTitle(void)
{
	static char temp[1000];

	memset(temp,0,1000);

	// 通常のファイル用 たまに変なファイルだと間違ったものを表示するが... //
	for(const auto& track : Mid_Seq.tracks) {
		const auto* p = track.data.data();
		while(!(p[0]==0xff && p[1]==0x2f && p[2]==0x00)){
			if(p[0]==0xff && p[1]==0x03){
				memcpy(temp,p+3,p[2]);
				return temp;
			}
			p++;
		}
	}

	// タイトルのはずなのに別のところに記述しているファイル用 //
	for(const auto& track : Mid_Seq.tracks) {
		const auto* p = track.data.data();
		while(!(p[0]==0xff && p[1]==0x2f && p[2]==0x00)){
			if(p[0]==0xff && p[1]==0x01){
				memcpy(temp,p+3,p[2]);
				return reinterpret_cast<const char *>(p+3);
			}
			p++;
		}
	}

	return temp;
}
