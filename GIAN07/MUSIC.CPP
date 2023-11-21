/*
 *   Music Room
 *
 */

#include "MUSIC.H"
#include "FONTUTY.H"
#include "platform/midi_backend.h"

std::optional<MUSICROOM_TEXT> MusicRoomText;

static constexpr RGBA ColorHighlight = {  51, 102, 153 };
static constexpr RGBA ColorDefault   = { 153, 204, 255 };

MUSICROOM_TEXT MUSICROOM_TEXT::Register()
{
	const MUSICROOM_TEXT ret = {
		.mid_dev = TextObj.Register({ 98, 13 }),
		.title = TextObj.Register({ 240, 16 }),
		.comment = TextObj.Register({ 272, 192 }),
		.version = TextObj.Register({ 490, 13 }),
	};

	TextObj.Prerender(ret.version, [](GIAN_TEXTRENDER_SESSION auto& s) {
		s.SetFont(GIAN_FONT_ID::SMALL);
		s.SetColor(ColorDefault);
		s.Put({ 0, 0 }, "秋霜玉    Version 1.005     ★デモ対応版＃★");
	});
	ret.PrerenderMidDev();

	return ret;
}

void MUSICROOM_TEXT::PrerenderMidDev() const
{
	TextObj.Prerender(mid_dev, [](GIAN_TEXTRENDER_SESSION auto& s) {
		const auto maybe_dev = MidBackend_DeviceName();
		if(!maybe_dev) {
			return;
		}
		const auto dev = maybe_dev.value();
		s.SetFont(GIAN_FONT_ID::SMALL);
		s.SetColor(ColorDefault);
		s.Put({ 0, 0 }, { dev.data(), (std::min)(dev.size(), 13u) });
	});
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
	TextObj.Prerender(title, [&](GIAN_TEXTRENDER_SESSION auto& s) {
		const auto maybe_title = cursor.next<LINE>();
		if(!maybe_title) {
			return;
		}
		const auto& title = maybe_title.value()[0];

		char buf[4 + sizeof(title.c) + 1];
		sprintf(buf, "#%02d %.26s", (number + 1), title.c);
		s.SetFont(GIAN_FONT_ID::NORMAL);
		s.Put({ 1, 0 }, buf, ColorHighlight);
		s.Put({ 0, 0 }, buf, ColorDefault);
	});

	TextObj.Prerender(comment, [&cursor](GIAN_TEXTRENDER_SESSION auto& s) {
		int y = 0;
		s.SetFont(GIAN_FONT_ID::SMALL);
		s.SetColor(ColorDefault);
		while(const auto line = cursor.next<LINE>()) {
			s.Put({ 0, y }, line.value()[0] );
			y += 16;
		}
	});
	return true;
}
