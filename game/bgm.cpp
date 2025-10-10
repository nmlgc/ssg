/*
 *   Format-independent background music interface
 *
 */

#include <SDL3/SDL_filesystem.h>

#include "game/bgm.h"
#include "game/bgm_track.h"
#include "game/defer.h"
#include "game/midi.h"
#include "game/snd.h"
#include "game/string_format.h"
#include "game/volume.h"
#include "platform/file.h"
#include "platform/midi_backend.h"
#include "platform/path.h"
#include "platform/snd_backend.h"

using namespace std::chrono_literals;

static constexpr std::u8string_view BGM_ROOT = u8"bgm/";
static constexpr std::u8string_view EXT_MID = u8".mid";

// State
// -----

uint8_t BGM_Tempo_Num = BGM_TEMPO_DENOM;

static bool Enabled = false;
static bool Playing = false;
static bool LoadedOriginalMIDI = false;
static bool GainApply = true;
static unsigned int LoadedNum = 0; // 0 = nothing
static std::chrono::milliseconds MIDITableUpdatePrev;

static std::optional<bool> PacksAvailable = std::nullopt;
static std::u8string PackPath;
static std::shared_ptr<BGM::TRACK> Waveform; // nullptr = playing MIDI
// -----

// External dependencies
// ---------------------

const uint8_t& Mid_TempoNum = BGM_Tempo_Num;
const uint8_t& Mid_TempoDenom = BGM_TEMPO_DENOM;

const uint8_t& Snd_BGMTempoNum = BGM_Tempo_Num;
const uint8_t& Snd_BGMTempoDenom = BGM_TEMPO_DENOM;
// ---------------------

#ifndef SUPPORT_MIDI_BACKEND
bool MidBackend_Init() { return false; }
void MidBackend_Cleanup() {}
bool MidBackend_DeviceChange(int8_t) { return false; }
void Mid_Play() {}
void Mid_Stop() {}
void Mid_Pause() {}
void Mid_Resume() {}
#endif

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

bool BGM_LoadedOriginalMIDI(void)
{
	return LoadedOriginalMIDI;
}

bool BGM_HasGainFactor(void)
{
	return (Waveform && Waveform->metadata.gain_factor.has_value());
}

bool BGM_GainApply(void)
{
	return GainApply;
}

BGM_PLAYING BGM_Playing(void)
{
	return (
		(!Enabled || !Playing) ? BGM_PLAYING::NONE :
		Waveform ? BGM_PLAYING::WAVEFORM :
		BGM_PLAYING::MIDI
	);
}

std::chrono::duration<int32_t, std::milli> BGM_PlayTime(void)
{
	if(Waveform) {
		return SndBackend_BGMPlayTime();
	}
	return Mid_PlayTime.realtime;
}

Narrow::string_view BGM_Title(void)
{
	auto ret = ((Waveform && !Waveform->metadata.title.empty())
		? Narrow::string_view(Waveform->metadata.title)
		: Mid_GetTitle()
	);

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
	if(ret && Playing && !Waveform) {
		Mid_Play();
	}
	return ret;
}

static bool BGM_Load(unsigned int id)
{
	if(!PackPath.empty()) {
		LoadedOriginalMIDI = false;
		const auto prefix_len = PackPath.size();
		StringCatNum<2>((id + 1), PackPath);

		// Try loading a waveform track
		bool waveform_new = false;
		bool mid_new = false;
		Waveform = BGM::TrackOpen(PackPath);
		if(Waveform) {
			if(SndBackend_BGMLoad(Waveform)) {
				waveform_new = true;
				if(const auto& hash = Waveform->metadata.source_midi) {
					mid_new = BGM_MidLoadByHash(*hash);
					LoadedOriginalMIDI = mid_new;
				}
			}
		}

		// Try loading a MIDI
		if(!mid_new) {
			PackPath += EXT_MID;
			mid_new = BGM_MidLoadBuffer(SDL_LoadFile(PackPath.c_str()));
		}

		PackPath.resize(prefix_len);
		if(waveform_new || mid_new) {
			return true;
		}
	}
	LoadedOriginalMIDI = BGM_MidLoadOriginal(id);
	return LoadedOriginalMIDI;
}

bool BGM_Switch(unsigned int id)
{
	if(!Enabled) {
		return false;
	}
	BGM_Stop();
	Waveform = nullptr;
	const auto ret = BGM_Load(id);
	if(ret) {
		LoadedNum = (id + 1);
		BGM_SetTempo(BGM_GetTempo());
		BGM_Play();
	}
	return ret;
}

void BGM_Play(void)
{
	BGM_SetGainApply(GainApply);
	if(Waveform) {
		SndBackend_BGMPlay();
		MIDITableUpdatePrev = decltype(MIDITableUpdatePrev)::zero();
	} else {
		Mid_Play();
	}
	Playing = true;
}

void BGM_Stop(void)
{
	if(Waveform) {
		SndBackend_BGMStop();

		// Just in case MIDI is running in update-only mode and the tables are
		// being observed. Would regularly be called by Mid_Stop().
		Mid_TableInit();
	} else {
		Mid_Stop();
	}
	Playing = false;
}

void BGM_Pause(void)
{
	// This is called when the window loses focus. Maybe the user will add a
	// BGM pack before coming back?
	PacksAvailable = std::nullopt;

	// Waveform tracks are automatically paused as part of the Snd subsystem
	// once the game window loses focus. We might need independent pausing in
	// the future?
	if(!Waveform) {
		Mid_Pause();
	}
}

void BGM_Resume(void)
{
	// Same as for pausing; /s/paus/resum/g, /s/loses/regains/
	if(!Waveform) {
		Mid_Resume();
	}
}

void BGM_UpdateMIDITables(void)
{
	if(Waveform && Playing) {
		// This is the only per-frame update function in the current
		// architecture that can actually stop playback at the end of a fade.
		// Coincidentally, the MIDI tables are the only place where this
		// matters, and true stopping could be observed.
		if(Waveform->FadeVolumeLinear() <= 0.0f) {
			BGM_Stop();
			return;
		}

		const auto now = SndBackend_BGMPlayTime();
		const auto delta = (now - MIDITableUpdatePrev);
		MIDITableUpdatePrev = now;
		Mid_Proc(delta);
	}
}

void BGM_SetGainApply(bool apply)
{
	GainApply = apply;
	if(Waveform) {
		Snd_BGMGainFactor = (apply
			? Waveform->metadata.gain_factor.value_or(1.0f)
			: 1.0f
		);
		SndBackend_BGMUpdateVolume();
	}
}

void BGM_UpdateVolume(void)
{
	if(Waveform) {
		SndBackend_BGMUpdateVolume();
	} else {
		Mid_UpdateVolume();
	}
}

void BGM_FadeOut(uint8_t speed)
{
	// pbg quirk: The original game always reduced the volume by 1 on the first
	// call to the MIDI FadeIO() method after the start of the fade. This
	// allowed you to hold the fade button in the Music Room for a faster
	// fade-out.
	const VOLUME volume_cur = (Waveform
		? VolumeDiscrete(Waveform->FadeVolumeLinear())
		: Mid_GetFadeVolume()
	);
	const auto volume_start = (volume_cur - 1);

	const auto duration = (
		10ms * VOLUME_MAX * ((((256 - speed) * 4) / (VOLUME_MAX + 1)) + 1)
	);
	if(Waveform) {
		Waveform->FadeOut(VolumeLinear(volume_start), duration);
	} else {
		Mid_FadeOut(volume_start, duration);
	}
}

int8_t BGM_GetTempo(void)
{
	return (BGM_Tempo_Num - BGM_TEMPO_DENOM);
}

void BGM_SetTempo(int8_t tempo)
{
	tempo = std::clamp(tempo, BGM_TEMPO_MIN, BGM_TEMPO_MAX);
	BGM_Tempo_Num = (BGM_TEMPO_DENOM + tempo);
	SndBackend_BGMUpdateTempo();
}

static bool BGM_PackIterator(std::invocable<std::u8string_view> auto callback)
{
	return SDL_EnumerateDirectory(
		std::bit_cast<const char *>(BGM_ROOT.data()),
		[](void *cb, const char *bgm_root, const char *basename) {
			char *fn = nullptr;
			if(SDL_asprintf(&fn, "%s%s", bgm_root, basename) < 0) {
				return SDL_ENUM_FAILURE;
			}
			defer(SDL_free(fn));
			if(!PathIsDirectory(std::bit_cast<const char8_t *>(fn))) {
				return SDL_ENUM_CONTINUE;
			}
			return (std::bit_cast<decltype(callback) *>(cb))->operator()(
				std::u8string_view{ std::bit_cast<const char8_t *>(basename) }
			);
		},
		&callback
	);
}

bool BGM_PacksAvailable(bool invalidate_cache)
{
	if(PacksAvailable.has_value() && !invalidate_cache) {
		return PacksAvailable.value();
	}
	PacksAvailable = BGM_PackIterator([](const std::u8string_view) {
		return SDL_ENUM_SUCCESS;
	});
	return PacksAvailable.value();
}

size_t BGM_PackCount(void)
{
	size_t ret = 0;
	BGM_PackIterator([&](const std::u8string_view) {
		ret++;
		return SDL_ENUM_CONTINUE;
	});
	return ret;
}

void BGM_PackForeach(void func(const std::u8string_view pack))
{
	BGM_PackIterator([&](const std::u8string_view pack) {
		func(pack);
		return SDL_ENUM_CONTINUE;
	});
}

bool BGM_PackSet(const std::u8string_view pack)
{
	const std::u8string_view cur = PackPath;
	if(!pack.empty()) {
		const auto path_data = PathForData();
		const auto root_len = (path_data.size() + BGM_ROOT.size());
		if(
			(cur.size() > root_len) &&
			(cur.substr(root_len, pack.size()) == pack) &&
			(cur[root_len + pack.size()] == '/') // !!!
		) {
			return true;
		}
		const auto pack_len = (pack.size() + 1);
		const auto file_len = (STRING_NUM_CAP<unsigned int> + EXT_MID.size());
		const auto len = (root_len + pack_len + file_len + 1);
		PackPath.resize_and_overwrite(len, [&](char8_t* buf, size_t len) {
			std::ranges::in_out_result p = { path_data.begin(), buf };
			p = std::ranges::copy(path_data, p.out);
			p = std::ranges::copy(BGM_ROOT, p.out);
			p = std::ranges::copy(pack, p.out);
			*(p.out++) = '/';
			return (p.out - buf);
		});

		// Check if this path exists
		if(!PathIsDirectory(PackPath.c_str())) {
			PackPath.clear();
			return false;
		}
	} else {
		if(cur.empty()) {
			return true;
		}
		PackPath.clear();
	}

	if((LoadedNum != 0) && Playing) {
		BGM_Switch(LoadedNum - 1);
	}
	return true;
}
