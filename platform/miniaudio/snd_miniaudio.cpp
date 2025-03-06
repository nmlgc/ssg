/*
 *   Sound via miniaudio
 *
 */

#include "platform/miniaudio/flags.h"
#include <libs/miniaudio/miniaudio.h>

#include "game/bgm_track.h"
#include "game/defer.h"
#include "platform/snd_backend.h"

struct BGM_OBJ {
	ma_data_source_base data_source{};
	ma_sound sound{};
	std::shared_ptr<BGM::TRACK> track = nullptr;

	bool Clear() {
		if(track) {
			ma_sound_uninit(&sound);
			ma_data_source_uninit(&data_source);
			track = nullptr;
		}
		return false;
	}
};

static_assert(
	(offsetof(BGM_OBJ, data_source) == 0),
	"The `ma_data_source_base` object must come first in the structure"
);

static ma_result BGM_GetDataFormat(
	ma_data_source* pDataSource,
	ma_format* pFormat,
	ma_uint32* pChannels,
	ma_uint32* pSampleRate,
	ma_channel* pChannelMap,
	size_t channelMapCap
) {
	const auto* bgm = static_cast<BGM_OBJ *>(pDataSource);
	if(!bgm->track) {
		return MA_UNAVAILABLE;
	}
	switch(bgm->track->pcmf.format) {
	case PCM_SAMPLE_FORMAT::S16:	*pFormat = ma_format_s16;	break;
	case PCM_SAMPLE_FORMAT::S32:	*pFormat = ma_format_s32;	break;
	}
	*pChannels = bgm->track->pcmf.channels;
	*pSampleRate = bgm->track->pcmf.samplingrate;
	return MA_SUCCESS;
}

static ma_result BGM_Read(
	ma_data_source* pDataSource,
	void* pFramesOut,
	ma_uint64 frameCount,
	ma_uint64* pFramesRead
) {
	const auto* bgm = static_cast<BGM_OBJ *>(pDataSource);
	if(!bgm->track) {
		return MA_UNAVAILABLE;
	}
	const auto sample_size = bgm->track->pcmf.SampleSize();

	if((frameCount * sample_size) > (std::numeric_limits<size_t>::max)()) {
		return MA_TOO_BIG;
	}
	const size_t buf_size = (frameCount * sample_size);
	if(!bgm->track->Decode({static_cast<std::byte *>(pFramesOut), buf_size})) {
		return MA_INVALID_DATA;
	}
	*pFramesRead = frameCount;
	return MA_SUCCESS;
}

static const ma_data_source_vtable BGM_VTABLE = {
	.onRead = BGM_Read,
	.onGetDataFormat = BGM_GetDataFormat,
};

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
static ma_sound_group SEGroup;
static BGM_OBJ BGMObj;
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

bool SndBackend_Init(void)
{
	return (ma_engine_init(nullptr, &Engine) == MA_SUCCESS);
}

void SndBackend_Cleanup(void)
{
	ma_engine_uninit(&Engine);
}

bool SndBackend_BGMInit(void)
{
	return true;
}

void SndBackend_BGMCleanup(void)
{
	BGMObj.Clear();
}

bool SndBackend_BGMLoad(std::shared_ptr<BGM::TRACK> track)
{
	ma_result result = MA_SUCCESS;

	BGMObj.Clear();
	BGMObj.track = track;

	auto config = ma_data_source_config_init();
	config.vtable = &BGM_VTABLE;

	result = ma_data_source_init(&config, &BGMObj.data_source);
	if(result != MA_SUCCESS) {
		return result;
	}
	result = ma_sound_init_from_data_source(
		&Engine,
		&BGMObj.data_source,
		MA_SOUND_FLAG_NO_SPATIALIZATION,
		nullptr,
		&BGMObj.sound
	);
	if(result != MA_SUCCESS) {
		return BGMObj.Clear();
	}
	return true;
}

void SndBackend_BGMPlay(void)
{
	if(!BGMObj.track) {
		return;
	}
	ma_sound_start(&BGMObj.sound);
}

void SndBackend_BGMStop(void)
{
	if(!BGMObj.track) {
		return;
	}
	ma_sound_stop(&BGMObj.sound);
}

std::chrono::milliseconds SndBackend_BGMPlayTime(void)
{
	if(!BGMObj.track) {
		return std::chrono::milliseconds::zero();
	}
	const auto ret = ma_sound_get_time_in_milliseconds(&BGMObj.sound);
	return std::chrono::milliseconds{ ret };
}

void SndBackend_BGMUpdateVolume(void)
{
	if(!BGMObj.track) {
		return;
	}
	ma_sound_set_volume(
		&BGMObj.sound, (Snd_BGMGainFactor * VolumeFactorSquare(Snd_VolumeBGM))
	);
}

void SndBackend_BGMUpdateTempo(void)
{
	if(!BGMObj.track) {
		return;
	}
	const auto t = (static_cast<float>(Snd_BGMTempoNum) / Snd_BGMTempoDenom);
	ma_sound_set_pitch(&BGMObj.sound, t);
}

bool SndBackend_SEInit(void)
{
	const auto ret = ma_sound_group_init(
		&Engine, MA_SOUND_FLAG_NO_PITCH, nullptr, &SEGroup
	);
	return (ret == MA_SUCCESS);
}

void SndBackend_SECleanup(void)
{
	for(auto& se : SndObj) {
		se.Clear();
	}
	ma_sound_group_uninit(&SEGroup);
}

void SndBackend_SEUpdateVolume(void)
{
	ma_sound_group_set_volume(&SEGroup, VolumeFactorSquare(Snd_VolumeSE));
}

bool SndBackend_SELoad(
	BYTE_BUFFER_OWNED buffer, uint8_t id, SND_INSTANCE_ID max
)
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
			&SEGroup,
			&instance.sound
		);
		if(result != MA_SUCCESS) {
			return se.Clear();
		}
	}
	return true;
}

void SndBackend_SEPlay(uint8_t id, int x, bool loop)
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

void SndBackend_SEStop(uint8_t id)
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

void SndBackend_PauseAll(void)
{
	ma_engine_stop(&Engine);
}

void SndBackend_ResumeAll(void)
{
	ma_engine_start(&Engine);
}
