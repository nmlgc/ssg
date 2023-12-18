/*
 *   Sound via miniaudio
 *
 */

#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_FLAC
#define MA_NO_MP3
#define MA_NO_NULL

// Slightly crispier than what you'd get with DirectSound, but 2 would already
// come with fewer high frequencies in comparison.
#define MA_DEFAULT_RESAMPLER_LPF_ORDER 1

#include <libs/miniaudio/miniaudio.h>

#include "game/defer.h"
#include "platform/snd.h"
#include <ranges>

struct RESAMPLED_INSTANCE {
	ma_audio_buffer_ref data_source{};
	ma_sound sound{};

	~RESAMPLED_INSTANCE() {
		ma_sound_uninit(&sound);
		ma_audio_buffer_ref_uninit(&data_source);
	}
};

struct SE {
	// Backing storage for the individual `ma_audio_buffer` instances.
	void* resampled_buffer = nullptr;

	std::unique_ptr<RESAMPLED_INSTANCE[]> instance;
	unsigned int max = 0;
	unsigned int now = 0;

	bool Clear() {
		max = 0;
		now = 0;
		instance = nullptr;
		ma_free(resampled_buffer, nullptr);
		resampled_buffer = nullptr;
		return false;
	}

	bool Loaded() {
		return (instance.get() != nullptr);
	}
};

static ma_engine Engine;
static SE SndObj[SND_OBJ_MAX];

float x_to_linear(int x)
{
	// The basic dB→linear conversion formula. (linear = 10 ^ (dB / 20))
	const auto x_rel = (x - SND_X_MID);
	const auto x_power = (x_rel / (SND_X_PER_DECIBEL * 20.0f));
	return ((x_rel < 0)
		? (powf(10.0f, x_power) - 1.0f)
		: (1.0f - powf(10.0f, -x_power))
	);
}

bool SndInit(void)
{
	return (ma_engine_init(nullptr, &Engine) == MA_SUCCESS);
}

void SndCleanup(void)
{
	for(auto& se : SndObj) {
		se.Clear();
	}
	ma_engine_uninit(&Engine);
}

bool Snd_SELoad(BYTE_BUFFER_OWNED buffer, uint8_t id, SND_INSTANCE_ID max)
{
	if(id >= SND_OBJ_MAX) {
		return false;
	}
	auto& se = SndObj[id];

	ma_result result = MA_SUCCESS;
	se.Clear();

	if(max == 0) {
		return false;
	}

	se.instance = std::unique_ptr<RESAMPLED_INSTANCE[]>(
		new (std::nothrow) RESAMPLED_INSTANCE[max]
	);
	if(!se.instance) {
		return false;
	}
	se.max = max;
	se.now = 0;

	// Resample to the native sample rate and create a single authoritative
	// buffer. We still keep the original channel count, though, since mono
	// expansion is SSE2-optimized.
	const auto* device = ma_engine_get_device(&Engine);
	auto config = ma_decoder_config_init(
		device->playback.format, 0, device->sampleRate
	);
	ma_uint64 frames;
	result = ma_decode_memory(
		buffer.get(), buffer.size(), &config, &frames, &se.resampled_buffer
	);
	if(result != MA_SUCCESS) {
		return se.Clear();
	}

	for(const auto i : std::views::iota(0u, se.max)) {
		auto& instance = se.instance[i];
		result = ma_audio_buffer_ref_init(
			config.format,
			config.channels,
			se.resampled_buffer,
			frames,
			&instance.data_source
		);
		if(result != MA_SUCCESS) {
			return se.Clear();
		}
		result = ma_sound_init_from_data_source(
			&Engine,
			&instance.data_source,
			(MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION),
			nullptr,
			&instance.sound
		);
		if(result != MA_SUCCESS) {
			return se.Clear();
		}
	}
	return true;
}

void Snd_SEPlay(uint8_t id, int x, bool loop)
{
	if(id >= SND_OBJ_MAX) {
		return;
	}
	auto& se = SndObj[id];
	if(!se.Loaded()) {
		return;
	}
	auto& instance = se.instance[se.now];

	// I *guess* we don't have to worry about thread safety here? These
	// operations are either atomic (loop, seek target, playback state), or
	// passed by value (pan). The worst thing that can happen is that these
	// changes only apply to the next iteration of the audio thread. Which is
	// probably still better than busy-waiting for the [isReading] lock of the
	// engine's node graph.
	ma_sound_stop(&instance.sound); // Both are necessary!
	ma_sound_set_looping(&instance.sound, loop);
	ma_sound_set_pan(&instance.sound, x_to_linear(x));
	ma_sound_seek_to_pcm_frame(&instance.sound, 0); // Both are necessary!
	ma_sound_start(&instance.sound);

	se.now = ((se.now + 1) % se.max);
}

void Snd_SEStop(uint8_t id)
{
	if(id >= SND_OBJ_MAX) {
		return;
	}
	auto& se = SndObj[id];
	for(const auto i : std::views::iota(0u, se.max)) {
		auto& instance = se.instance[i];
		ma_sound_stop(&instance.sound);
	}
}

void SndPauseAll(void)
{
	ma_engine_stop(&Engine);
}

void SndResumeAll(void)
{
	ma_engine_start(&Engine);
}
