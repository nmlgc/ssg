/*
 *   Music Room
 *
 */

#include "MUSIC.H"
#include "game/bgm.h"
#include "game/string_format.h"
#include "platform/midi_backend.h"
#include "platform/text_backend.h"

std::optional<MUSICROOM_TEXT> MusicRoomText;

static constexpr RGB ColorHighlight = {  51, 102, 153 };
static constexpr RGB ColorDefault   = { 153, 204, 255 };

MUSICROOM_TEXT MUSICROOM_TEXT::Register()
{
	const MUSICROOM_TEXT ret = {
		.mid_dev = TextObj.Register({ 98, 13 }),
		.title = TextObj.Register({ 240, 16 }),
		.comment = TextObj.Register({ 272, 192 }),
		.version = TextObj.Register({ 490, 13 }),
	};

	TextObj.Prerender(ret.version, [](TEXTRENDER_SESSION auto& s) {
		s.SetFont(FONT_ID::SMALL);
		s.SetColor(ColorDefault);
		s.Put({ 0, 0 }, "秋霜玉    Version 1.005     ★デモ対応版＃★");
	});
	ret.PrerenderMidDev();

	return ret;
}

void MUSICROOM_TEXT::PrerenderMidDev() const
{
#ifdef SUPPORT_MIDI_BACKEND
	TextObj.Prerender(mid_dev, [](TEXTRENDER_SESSION auto& s) {
		const auto maybe_dev = MidBackend_DeviceName();
		if(!maybe_dev) {
			return;
		}
		const auto dev = maybe_dev.value();
		s.SetFont(FONT_ID::SMALL);
		s.SetColor(ColorDefault);
		s.Put({ 0, 0 }, { dev.data(), (std::min)(dev.size(), 13u) });
	});
#endif
}

bool MUSICROOM_TEXT::PrerenderTitleAndComment(
	BYTE_BUFFER_OWNED buffer, unsigned int number
) const
{
	struct LINE {
		char c[19 * 2];

		operator Narrow::string_view() const {
			return { c, sizeof(c) };
		}
	};

	auto cursor = buffer.cursor();
	TextObj.Prerender(title, [&](TEXTRENDER_SESSION auto& s) {
		// Skip the title in the comment and take it from the BGM instead.
		cursor.next<LINE>();

		const auto& title = BGM_Title();
		Narrow::string buf;
		const auto len = (
			1 + STRING_NUM_CAP<decltype(number)> + 1 + title.size()
		);
		buf.resize_and_overwrite(len, [&](decltype(buf)::value_type* p, auto) {
			const auto* start = p;
			*(p++) = '#';
			p += sprintf(p, "%02u", (number + 1));
			*(p++) = ' ';
			p = std::ranges::copy(title, p).out;
			return (p - start);
		});
		s.SetFont(FONT_ID::NORMAL);
		s.Put({ 1, 0 }, buf, ColorHighlight);
		s.Put({ 0, 0 }, buf, ColorDefault);
	});

	TextObj.Prerender(comment, [&cursor](TEXTRENDER_SESSION auto& s) {
		if(!BGM_LoadedOriginalMIDI()) {
			return;
		}
		int y = 0;
		s.SetFont(FONT_ID::SMALL);
		s.SetColor(ColorDefault);
		while(const auto line = cursor.next<LINE>()) {
			s.Put({ 0, y }, line.value()[0] );
			y += 16;
		}
	});
	return true;
}
