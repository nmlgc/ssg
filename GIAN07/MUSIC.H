/*
 *   Music Room
 *
 */

#pragma once

#include "game/text.h"
#include "platform/buffer.h"

struct MUSICROOM_TEXT {
	TEXTRENDER_RECT_ID mid_dev;
	TEXTRENDER_RECT_ID title;
	TEXTRENDER_RECT_ID comment;
	TEXTRENDER_RECT_ID version;

	static MUSICROOM_TEXT Register();

	void PrerenderMidDev() const;
	bool PrerenderTitleAndComment(
		BYTE_BUFFER_OWNED buffer, unsigned int number
	) const;
};
extern std::optional<MUSICROOM_TEXT> MusicRoomText;

// 音楽室の初期化をする
bool MusicRoomInit(void);

// 音楽室
void MusicRoomProc(bool&);
