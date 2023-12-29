/*
 *   Format-independent background music interface
 *
 */

#include "game/bgm.h"
#include "game/midi.h"
#include "game/snd.h"
#include "game/volume.h"
#include <algorithm>

using namespace std::chrono_literals;

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

void BGM_FadeOut(uint8_t speed)
{
	// pbg quirk: The original game always reduced the volume by 1 on the first
	// call to the MIDI FadeIO() method after the start of the fade. This
	// allowed you to hold the fade button in the Music Room for a faster
	// fade-out.
	const auto volume_start = (Mid_GetFadeVolume() - 1);

	const auto duration = (
		10ms * VOLUME_MAX * ((((256 - speed) * 4) / (VOLUME_MAX + 1)) + 1)
	);
	Mid_FadeOut(volume_start, duration);
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
