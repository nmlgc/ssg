/*
 *   PCM sound interface
 *
 */

#pragma once


// ヘッダファイル //
#include "game/enum_flags.h"
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

extern const uint8_t& Snd_BGMTempoNum;
extern const uint8_t& Snd_BGMTempoDenom;

void Snd_Cleanup(void);

// BGM
// ---

extern float Snd_BGMGainFactor;

bool Snd_BGMInit(void);
void Snd_BGMCleanup(void);
// ---

bool Snd_SEInit(void);
void Snd_SECleanup(void);

bool Snd_SELoad(BYTE_BUFFER_OWNED buffer, uint8_t id, SND_INSTANCE_ID max);

// 再生＆停止 //
void Snd_SEPlay(uint8_t id, int x = SND_X_MID, bool loop = false);
void Snd_SEStop(uint8_t id);
void Snd_SEStopAll(void);
