/*
 *   Format-independent background music interface
 *
 */

#include "game/bgm.h"
#include "game/midi.h"
#include "game/snd.h"
#include <algorithm>

// State
// -----

uint8_t BGM_Tempo_Num = BGM_TEMPO_DENOM;
// -----

// External dependencies
// ---------------------

const uint8_t& Mid_TempoNum = BGM_Tempo_Num;
const uint8_t& Mid_TempoDenom = BGM_TEMPO_DENOM;
// ---------------------

bool BGM_Init(void)
{
	BGM_SetTempo(0);
	return (Mid_Start() && Snd_BGMInit());
}

int8_t BGM_GetTempo(void)
{
	return (BGM_Tempo_Num - BGM_TEMPO_DENOM);
}

void BGM_SetTempo(int8_t tempo)
{
	tempo = std::clamp(tempo, BGM_TEMPO_MIN, BGM_TEMPO_MAX);
	BGM_Tempo_Num = (BGM_TEMPO_DENOM + tempo);
}
