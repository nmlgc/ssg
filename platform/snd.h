/*
 *   PCM sound interface
 *
 */

#pragma once


// ヘッダファイル //
#include "platform/buffer.h"
#include <stdint.h>

// 定数＆マクロ //
using SND_INSTANCE_ID = uint8_t;

#define SND_OBJ_MAX			30				// 効果音の種類の最大数

// Constants for mapping the [x] parameter from any source unit to a position
// in the stereo field. The resulting unit is the attenuation volume of either
// the right (negative) or left (positive) channel, expressed in decibels.
extern const int SND_X_MID;
extern const int SND_X_PER_DECIBEL;


// 初期化など //
bool SndInit(void);
void SndCleanup(void);

bool SndWAVLoad(BYTE_BUFFER_OWNED buffer, uint8_t id, SND_INSTANCE_ID max);

// 再生＆停止 //
void SndPlay(uint8_t id, int x = SND_X_MID, bool loop = false);
void SndStop(uint8_t id);

inline void SndStopAll(void)
{
	for(auto i = 0; i < SND_OBJ_MAX; i++) {
		SndStop(i);
	}
}
