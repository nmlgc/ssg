/*
 *   PCM sound interface
 *
 */

#pragma once


// ヘッダファイル //
#include "game/enum_flags.h"
#include "game/volume.h"
#include "platform/buffer.h"
#include "constants.h"

// 定数＆マクロ //
using SND_INSTANCE_ID = uint8_t;

#define SND_OBJ_MAX			30				// 効果音の種類の最大数

extern const uint8_t& Snd_BGMTempoNum;
extern const uint8_t& Snd_BGMTempoDenom;

extern const VOLUME& Snd_VolumeBGM;
extern const VOLUME& Snd_VolumeSE;

void Snd_Cleanup(void);
void Snd_UpdateVolumes(void);

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
