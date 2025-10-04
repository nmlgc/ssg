/*
 *   Music Room
 *
 */

// GCC 15 throws `error: conflicting declaration 'typedef struct imaxdiv_t
// imaxdiv_t'` if this appears after a module import.
#include <inttypes.h> // for PRId64

#include "MUSIC.H"
#include "EFFECT.H"
#include "FONTUTY.H"
#include "GAMEMAIN.H"
#include "LOADER.H"
#include "game/bgm.h"
#include "game/debug.h"
#include "game/input.h"
#include "game/midi.h"
#include "game/string_format.h"
#include "game/text.h"
#include "game/ut_math.h"
#include "platform/midi_backend.h"
#include "platform/text_backend.h"

extern bool IsDraw();

// Constants
// ---------

static constexpr RGB ColorHighlight = {  51, 102, 153 };
static constexpr RGB ColorDefault   = { 153, 204, 255 };
// ---------

// State
// -----

struct MUSICROOM_TEXT {
	TEXTRENDER_RECT_ID mid_dev;
	TEXTRENDER_RECT_ID title;
	TEXTRENDER_RECT_ID comment;
	TEXTRENDER_RECT_ID version;
	BYTE_BUFFER_OWNED comment_buf = nullptr;

	void RenderVersion(WINDOW_POINT topleft) const;
	void RenderMidDev(WINDOW_POINT topleft) const;
	void RenderTitle(WINDOW_POINT topleft) const;
	void RenderComment(WINDOW_POINT topleft) const;
};

decltype(MusicNum) MidiPlayID = 0;
std::optional<MUSICROOM_TEXT> MusicRoomText;
// -----

void MUSICROOM_TEXT::RenderVersion(WINDOW_POINT topleft) const
{
	static constexpr Narrow::string_view VERSION = (
		"秋霜玉    Version 1.005     ★デモ対応版＃★"
	);
	TextObj.Render(topleft, version, VERSION, [](TEXTRENDER_SESSION& s) {
		s.SetFont(FONT_ID::SMALL);
		s.SetColor(ColorDefault);
		s.Put({ 0, 0 }, VERSION);
	});
}

void MUSICROOM_TEXT::RenderMidDev(WINDOW_POINT topleft) const
{
#ifdef SUPPORT_MIDI_BACKEND
	const auto maybe_dev_full = MidBackend_DeviceName();
	if(!maybe_dev_full) {
		return;
	}
	const auto dev_full = maybe_dev_full.value();
	const Narrow::string_view dev = {
		dev_full.data(), (std::min)(dev_full.size(), 13u)
	};
	TextObj.Render(topleft, mid_dev, dev, [&dev](TEXTRENDER_SESSION& s) {
		s.SetFont(FONT_ID::SMALL);
		s.SetColor(ColorDefault);
		s.Put({ 0, 0 }, dev);
	});
#endif
}

void MUSICROOM_TEXT::RenderTitle(WINDOW_POINT topleft) const
{
	// Some modders might assign the same title to consecutive tracks, but it's
	// not possible to change the track title without switching to a different
	// track first.
	char num_buf[1 + STRING_NUM_CAP<decltype(MidiPlayID)> + 1];
	const size_t num_len = sprintf(num_buf, "#%02u", (MidiPlayID + 1));
	Narrow::string_view num = { num_buf, num_len };

	TextObj.Render(topleft, title, num, [&num](TEXTRENDER_SESSION& s) {
		const auto& title = BGM_Title();

		// GDI would calculate a trailing space as 4 pixels wide, not 8.
		const auto title_left = (s.Extent(num).w + 8);

		s.SetFont(FONT_ID::NORMAL);
		s.Put({ 1, 0 }, num, ColorHighlight);
		s.Put({ (title_left + 1), 0 }, title, ColorHighlight);
		s.Put({ 0, 0 }, num, ColorDefault);
		s.Put({ (title_left + 0), 0 }, title, ColorDefault);
	});
}

void MUSICROOM_TEXT::RenderComment(WINDOW_POINT topleft) const
{
	struct LINE {
		char c[19 * 2];

		operator Narrow::string_view() const {
			return { c, sizeof(c) };
		}
	};

	Narrow::string_view comment_str = {};
	BYTE_BUFFER_CURSOR<const uint8_t> cursor = { std::span<const uint8_t>() };
	if(BGM_LoadedOriginalMIDI()) {
		// Skip the title in the comment
		cursor = comment_buf.cursor();
		cursor.next<LINE>();
		comment_str = {
			(std::bit_cast<const char *>(comment_buf.get()) + cursor.cursor),
			(comment_buf.size() - cursor.cursor),
		};
	}
	TextObj.Render(topleft, comment, comment_str, [&](TEXTRENDER_SESSION& s) {
		int y = 0;
		s.SetFont(FONT_ID::SMALL);
		s.SetColor(ColorDefault);
		while(const auto line = cursor.next<LINE>()) {
			s.Put({ 0, y }, line.value()[0] );
			y += 16;
		}
	});
}

bool MusicRoomInit(void)
{
	TextObj.Clear();
	GrpBackend_Clear();
	Grp_Flip();

	if(!LoadGraph(GRAPH_ID_MUSICROOM)) {
		DebugOut(u8"GRAPH.DAT が破壊されています");
		return false;
	}

	GrpBackend_SetClip(GRP_RES_RECT);

	MidiPlayID = 0;

	BGM_SetTempo(0);

	// Still necessary because the note arrays aren't actually processed
	// outside of the Music Room.
	Mid_TableInit();

	// BGM_Stop();
	auto comment_buf = LoadMusicRoomComment(0);
	if(!comment_buf) {
		DebugOut(u8"ENEMY.DAT がはかいされています");
		GameExit();
		return false;
	}

	MusicRoomText = MUSICROOM_TEXT
	{
		.mid_dev = TextObj.Register({  98,  13 }),
		.title   = TextObj.Register({ 240,  16 }),
		.comment = TextObj.Register({ 272, 192 }),
		.version = TextObj.Register({ 490,  13 }),

		.comment_buf = std::move(comment_buf),
	};

/*
	if(!LoadMusic(0)) {
		DebugOut(u8"MUSIC.DAT が破壊されています");
		GameExit();
		return false;
	}
*/
	// BGM_Play();

	GameMain = MusicRoomProc;

	return true;
}

// スペアナ描画 //
void GrpDrawSpect(int x, int y)
{
	uint16_t ftable[128 + 8 + 8], ftable2[128];

	static uint16_t ftable3[128 + 8 + 8];
	static uint8_t ftable3flag;

	constexpr PIXEL_LTRB src = {
		(16 * 16), 0, ((16 * 16) + (8 * 21)), 8
	}; // ,,,8*4

	ftable3flag = ((ftable3flag + 1) % 5);

	for(int i = 0; i < std::size(ftable2); i++) {
		int temp = 0;
		int temp2 = 0;
		for(const auto j : std::views::iota(0, 16)) {
			temp += Mid_PlayTable[j][i];
			temp2 += (Mid_PlayTable[j][i] ? 1 : 0);
			if(Mid_PlayTable[j][i] != 0) {
				Mid_PlayTable[j][i] -= ((Mid_PlayTable[j][i] >> 3) + 1); //4
			}
			//if(Mid_PlayTable[j][i]) Mid_PlayTable[j][i]-=(Mid_PlayTable[j][i]>>3)+1;
		}
		if(temp2 == 0) {
			temp2 = 1;
		}
		ftable2[i] = (temp / temp2);
	}

	for(int i = 0; i < std::size(ftable); i++) {
		ftable[i]  = ((i>=8 && i<=128+7) ? ftable2[i-8] : 0)   *(256-sinm(0))/256;

		ftable[i] += ((i>=9  && i<=128+8 ) ? ftable2[i-9 ] : 0)*(256-sinm( 8))/256;
		ftable[i] += ((i>=10 && i<=128+9 ) ? ftable2[i-10] : 0)*(256-sinm(16))/256;
		ftable[i] += ((i>=11 && i<=128+10) ? ftable2[i-11] : 0)*(256-sinm(24))/256;
		ftable[i] += ((i>=12 && i<=128+11) ? ftable2[i-12] : 0)*(256-sinm(32))/256;
		ftable[i] += ((i>=13 && i<=128+12) ? ftable2[i-13] : 0)*(256-sinm(40))/256;
		ftable[i] += ((i>=14 && i<=128+13) ? ftable2[i-14] : 0)*(256-sinm(48))/256;
		ftable[i] += ((i>=15 && i<=128+14) ? ftable2[i-15] : 0)*(256-sinm(56))/256;
		ftable[i] += ((i>=16 && i<=128+15) ? ftable2[i-16] : 0)*(256-sinm(63))/256;

		ftable[i] += ((i<=128+6 && i>=7) ? ftable2[i-7] : 0) *(256-sinm( 8))/256;
		ftable[i] += ((i<=128+5 && i>=6) ? ftable2[i-6] : 0) *(256-sinm(16))/256;
		ftable[i] += ((i<=128+4 && i>=5) ? ftable2[i-5] : 0) *(256-sinm(24))/256;
		ftable[i] += ((i<=128+3 && i>=4) ? ftable2[i-4] : 0) *(256-sinm(32))/256;
		ftable[i] += ((i<=128+2 && i>=3) ? ftable2[i-3] : 0) *(256-sinm(40))/256;
		ftable[i] += ((i<=128+1 && i>=2) ? ftable2[i-2] : 0) *(256-sinm(48))/256;
		ftable[i] += ((i<=128+0 && i>=1) ? ftable2[i-1] : 0) *(256-sinm(56))/256;
		ftable[i] += ((i<=128-1 && i>=0) ? ftable2[i-0] : 0) *(256-sinm(63))/256;

		ftable[i] >>= 3;

		if(ftable3[i] < ftable[i]) {
			ftable3[i] = ftable[i];
		} else if(!ftable3flag && ftable3[i]) {
			ftable3[i]--;
		}
	}

	// GrpSurface_Blit({ (SPECT_X - 7), SPECT_Y }, SURFACE_ID::SYSTEM, src);

	GrpGeom->Lock();

	if(auto *gp = GrpGeom_Poly()) {
		for(int i = 0; i < std::size(ftable); i++) {
			//WORD c2 = 0;	//5
			constexpr RGB c1 = { 200,   0, 0 };
			constexpr RGB c2 = { 250, 250, 0 };
			gp->DrawGrdLineEx((i + x), (y - (ftable[i] * 2)), c1, y, c2);
		}
	} else if(auto *gf = GrpGeom_FB()) {
		gf->SetColor({ 4, 2, 1 });
		for(int i = 0; i < std::size(ftable); i++) {
			//WORD c2 = 0;	//5
			if(ftable[i]) {
				gf->DrawLine((i + x), (y - (ftable[i] * 2)), (i + x), y);
			}
		}
	}

	GrpGeom->Unlock();
}

// 押されているところを表示 //
void GrpDrawNote(void)
{
	// 0123456789ab (Mod c)
	// o#o#oo#o#o#o
	// o o oo o o o

	constexpr const PIXEL_LTRB src[12] = {
		{  0, 464,  3, 474 },	// しろ
		{  0, 456,  3, 461 },	// 黒

		{  4, 464,  7, 474 },	// しろ
		{  0, 456,  3, 461 },	// 黒

		{  8, 464, 11, 474 },	// しろ
		{ 12, 464, 15, 474 },	// しろ

		{  0, 456,  3, 461 },	// 黒
		{ 16, 464, 19, 474 },	// しろ

		{  0, 456,  3, 461 },	// 黒
		{ 20, 464, 23, 474 },	// しろ

		{  0, 456,  3, 461 },	// 黒
		{ 24, 464, 27, 474 },	// しろ
	};

	constexpr const PIXEL_LTRB src2[12] = {
		{  0, (464 - 24),  3, (474 - 24) },	// しろ
		{  0, (456 - 24),  3, (461 - 24) },	// 黒

		{  4, (464 - 24),  7, (474 - 24) },	// しろ
		{  0, (456 - 24),  3, (461 - 24) },	// 黒

		{  8, (464 - 24), 11, (474 - 24) },	// しろ
		{ 12, (464 - 24), 15, (474 - 24) },	// しろ

		{  0, (456 - 24),  3, (461 - 24) },	// 黒
		{ 16, (464 - 24), 19, (474 - 24) },	// しろ

		{  0, (456 - 24),  3, (461 - 24) },	// 黒
		{ 20, (464 - 24), 23, (474 - 24) },	// しろ

		{  0, (456 - 24),  3, (461 - 24) },	// 黒
		{ 24, (464 - 24), 27, (474 - 24) },	// しろ
	};

	constexpr const PIXEL_COORD destX[12] = {
		0,	// しろ
		2,	// 黒

		4,	// しろ
		6,	// 黒

		8,	// しろ
		12,	// しろ

		14,	// 黒
		16,	// しろ

		18,	// 黒
		20,	// しろ

		22,	// 黒
		24,	// しろ
	};

	PIXEL_LTRB rc;

	for(const auto Track : std::views::iota(0, 16)) {
		const auto top = (22 + (Track * 24));
		const auto pan = (Cast::sign<int8_t>(Mid_PanpodTable[Track]) - 64);
		GrpPutMidNum( 50, top, Mid_VolumeTable[Track]);
		GrpPutMidNum(125, top, Mid_ExpressionTable[Track]);
		GrpPutMidNum(181, top, pan);

		int LevelSum = 0;
		int num = 0;
		for(const auto NoteNo : std::views::iota(0, 128)) {
			if(Mid_NoteWTable[Track][NoteNo]) {
				const auto x = (40 + destX[NoteNo % 12] + ((NoteNo / 12) * 28));
				const auto y = ( 9 + (Track * 24));
				GrpSurface_Blit({ x, y }, SURFACE_ID::MUSIC, src[NoteNo % 12]);
				Mid_NoteWTable[Track][NoteNo]--;
			}

			if(Mid_NoteTable[Track][NoteNo]) {
				const auto x = (40 + destX[NoteNo % 12] + ((NoteNo / 12) * 28));
				const auto y = ( 9 + (Track * 24));
				GrpSurface_Blit({ x, y }, SURFACE_ID::MUSIC, src[NoteNo % 12]);
			}
			if(Mid_PlayTable2[Track][NoteNo]) {
				LevelSum += (Mid_PlayTable2[Track][NoteNo]);
				//if(Mid_PlayTable2[Track][NoteNo]>128) Mid_PlayTable2[Track][NoteNo]=128;
				Mid_PlayTable2[Track][NoteNo] -= (std::max)(
					(Mid_PlayTable2[Track][NoteNo] / 50), 1
				);
				num++;
			}
		}

		if(num) {
			rc = PIXEL_LTWH{ 80, 456, (std::min)((LevelSum / num), 96), 5 };
			GrpSurface_Blit(
				{ 240, (22 + (Track * 24)) }, SURFACE_ID::MUSIC, rc
			);
		}
	}
}

void MusicRoomProc(bool&)
{
	if(!MusicRoomText) {
		assert(!"Music Room not initialized?");
		std::unreachable();
	}
	auto& text = MusicRoomText.value();

	char buf[100];
	static decltype(Key_Data) Old_Key;
	static bool DevChgWait;

	const auto playing = BGM_Playing();

	if(Key_Data != Old_Key) {
		if(Input_IsCancel(Key_Data)) {
			DevChgWait = false;
			MusicRoomText = std::nullopt;
			GameExit();
			return;
		}
		if((Key_Data == KEY_RIGHT) || (Key_Data == KEY_LEFT)) {
			if(Key_Data == KEY_RIGHT) {
				MidiPlayID += 2;
			}
			BGM_Stop();
			MidiPlayID = ((MidiPlayID + MusicNum - 1) % MusicNum);
			BGM_Switch(MidiPlayID);
			text.comment_buf = LoadMusicRoomComment(MidiPlayID);
		}
		Old_Key = Key_Data;
	}

	switch(Key_Data) {
	case KEY_UP:    BGM_SetTempo(BGM_GetTempo() + 1); break;
	case KEY_DOWN:  BGM_SetTempo(BGM_GetTempo() - 1); break;
	case KEY_SHIFT: BGM_SetTempo(0); break;
	}

	if(SystemKey_Data & SYSKEY_BGM_FADE) {
		BGM_FadeOut(120);
	}

	BGM_UpdateMIDITables();

	if((playing == BGM_PLAYING::MIDI) && (SystemKey_Data & SYSKEY_BGM_DEVICE)) {
		if(!DevChgWait) {
			BGM_ChangeMIDIDevice(1);
			DevChgWait = true;
		}
	} else {
		// 押されていなければ再び有効化する //
		DevChgWait = false;
	}

	if(IsDraw()) {
		GrpBackend_Clear();

		auto BlitBG = [](const PIXEL_LTWH& rect) {
			GrpSurface_Blit({ rect.left, rect.top }, SURFACE_ID::MUSIC, rect);
		};

		auto BlitLegend = [](const PIXEL_LTWH& rect) {
			const PIXEL_LTRB src = (rect + PIXEL_POINT{ 0, 392 });
			GrpSurface_Blit(
				{ (8 + rect.left), (410 + rect.top) }, SURFACE_ID::MUSIC, src
			);
		};

		BlitBG({   0,   0, 504, 392 });	// From keyboard to spectrum analyzer
		BlitBG({ 504,   0, 136,  59 });	// Down to PASSED TIME
		BlitBG({ 504, 108, 136, 284 });	// Everything below MIDI DEVICE

		BlitLegend({   0,  0, 176,  40 });	// Left side
		BlitLegend({ 176, 11, 464,  29 });	// Right side without device key

		if(playing == BGM_PLAYING::MIDI) {
			BlitBG({ 504, 83, 136, 25 });   	// MIDI DEVICE
			BlitLegend({ 176, 0, 176, 11 });	// Device change key
		}

		//GrpDrawSpect(0,480);
		GrpDrawSpect(352, 128);
		GrpDrawNote();

		const auto millis = BGM_PlayTime().count();
		const auto m = ((millis / 1000) / 60);
		const auto s = ((millis / 1000) % 60);
		sprintf(buf,"%02d : %02d",m,s);
		GrpPut7B(560, 44, buf);
		//TextOut(hdc,561,40+2,buf,strlen(buf));

		if(Mid_Loaded()) {
			BlitBG({ 504,  59, 136,  24 });	// MIDI TIMER
			sprintf(buf, "%07" PRId64, Mid_PlayTime.pulse_interpolated);
			GrpPut7B(560, 68, buf);
			//TextOut(hdc,561,64+2,buf,strlen(buf));
		}

		sprintf(buf, "%3d", BGM_GetTempo());
		GrpPut7B(560, 116, buf);
		//TextOut(hdc,561,112+2,buf,strlen(buf));
		//SetTextColor(hdc,RGB(255*5/5,255*2/5,255*1/5));

#ifdef SUPPORT_MIDI_BACKEND
		if(playing == BGM_PLAYING::MIDI) {
			text.RenderMidDev({ (540 + 2), (96 - 3) });
		}
#endif
		text.RenderTitle({    400,       (144 +  2) });
		text.RenderComment({ (400 - 40), (144 + 30) });
		text.RenderVersion({ (200 - 50),  460       });

		Grp_Flip();
	}
}
