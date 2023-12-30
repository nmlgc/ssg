/*
 *   Format-independent background music interface
 *
 */

#include "game/bgm.h"
#include "game/midi.h"
#include "game/snd.h"
#include "game/volume.h"
#include "platform/midi_backend.h"
#include <algorithm>

using namespace std::chrono_literals;

// State
// -----

uint8_t BGM_Tempo_Num = BGM_TEMPO_DENOM;

static bool Enabled = false;
// -----

// External dependencies
// ---------------------

const uint8_t& Mid_TempoNum = BGM_Tempo_Num;
const uint8_t& Mid_TempoDenom = BGM_TEMPO_DENOM;
// ---------------------

bool BGM_Init(void)
{
	BGM_SetTempo(0);
	Enabled = (MidBackend_Init() | Snd_BGMInit());
	return Enabled;
}

void BGM_Cleanup(void)
{
	BGM_Stop();
	MidBackend_Cleanup();
	Snd_BGMCleanup();
	Enabled = false;
}

bool BGM_Enabled(void)
{
	return Enabled;
}

std::chrono::duration<int32_t, std::milli> BGM_PlayTime(void)
{
	return Mid_PlayTime.realtime;
}

Narrow::string_view BGM_Title(void)
{
	auto ret = Mid_GetTitle();

	// pbg bug: Four of the original track titles start with leading fullwidth
	// spaces:
	//
	// 	#04: "　　　幻想帝都"
	// 	#07: "　　天空アーミー"
	// 	#11: "　魔法少女十字軍"
	// 	#16: "　　シルクロードアリス"
	//
	// This looks like it was done on purpose to center the titles within the
	// 216 maximum pixels that the original code designated for the in-game
	// animation. However:
	//
	// • None of those actually has the correct amount of spaces that would
	//   have been required for exact centering.
	// • If pbg intended to center all the tracks, there should have been
	//   leading whitespace in 14 of the track titles, and not just in 4.
	//
	// Since the in-game animation code does clearly intend these titles to be
	// right-aligned, it makes more sense to just remove all leading
	// whitespace. Doing this here will also benefit the Music Room.
	const auto trim_leading = [](auto& str, Narrow::string_view prefix) {
		const auto ret = str.starts_with(prefix);
		if(ret) {
			str.remove_prefix(prefix.size());
		}
		return ret;
	};
	while(trim_leading(ret, " ") || trim_leading(ret, "\x81\x40")) {
	};

	// The original MIDI sequence titles also come with lots of *trailing*
	// whitespace. Adding 1 also turns `npos` to 0.
	ret = ret.substr(0, (ret.find_last_not_of(" ") + 1));

	return ret;
}

bool BGM_ChangeMIDIDevice(int8_t direction)
{
	// 各関数に合わせて停止処理を行う //
	Mid_Stop();

	const auto ret = MidBackend_DeviceChange(direction);
	if(ret) {
		Mid_Play();
	}
	return ret;
}

static bool BGM_Load(unsigned int id)
{
	return BGM_MidLoadOriginal(id);
}

bool BGM_Switch(unsigned int id)
{
	if(!Enabled) {
		return false;
	}
	BGM_Stop();
	const auto ret = BGM_Load(id);
	if(ret) {
		BGM_Play();
	}
	return ret;
}

void BGM_Play(void)
{
	Mid_Play();
}

void BGM_Stop(void)
{
	Mid_Stop();
}

void BGM_Pause(void)
{
	Mid_Pause();
}

void BGM_Resume(void)
{
	Mid_Resume();
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
