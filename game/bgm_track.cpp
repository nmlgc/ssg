/*
 *   BGM tracks that are rendered as PCM and output via the Snd subsystem
 *
 *   Adapted from thcrap's bgmmod module.
 */

#include "game/bgm_track.h"
#include "game/narrow.h"
#include "game/volume.h"
#include <assert.h>
#include <version> // need the library feature test macros...

#if(__cpp_lib_to_chars < 201611L)
#include <locale.h>
#include <stdlib.h>
#endif

namespace BGM {

void OnVorbisComment(METADATA_CALLBACK func, const std::u8string_view comment)
{
	const auto eq_i = comment.find('=');
	if(eq_i == std::u8string_view::npos) {
		return;
	}
	func(comment.substr(0, eq_i), comment.substr(eq_i + 1));
}

void ApplyVolumeError(std::span<std::byte>, uint16_t, TRACK_VOL&)
{
	assert(!"missing fade implementation");
}

void TRACK_VOL::SetVolumeLinear(float v)
{
	volume_linear = v;
	volume_factor = VolumeFactorSquare(v);
}

template <class BitDepth> static void ApplyVolume(
	std::span<std::byte> buf, uint16_t channels, TRACK_VOL& vol
)
{
	const auto samples = std::span<BitDepth>{
		reinterpret_cast<BitDepth *>(buf.data()),
		(buf.size_bytes() / sizeof(BitDepth))
	};
	auto it = samples.begin();

	// Slow path if we're fading
	if(vol.fade_remaining > 0) {
		while((vol.fade_remaining > 0) && (it != samples.end())) {
			for(decltype(channels) c = 0; c < channels; c++) {
				*(it++) *= vol.FadeVolumeFactor();
			}
			vol.fade_remaining--;
			vol.SetVolumeLinear(vol.fade_end +
				((vol.fade_delta * vol.fade_remaining) / vol.fade_duration)
			);
		}
	}

	// Fast path for constant volume
	while(it != samples.end()) {
		*(it++) *= vol.FadeVolumeFactor();
	}
}

bool TRACK::Decode(std::span<std::byte> buf)
{
	size_t offset = 0;
	auto size_left = buf.size_bytes();
	while(size_left > 0) {
		const auto ret = DecodeSingle(buf.subspan(offset, size_left));
		if(ret == static_cast<size_t>(-1)) {
			std::ranges::fill(buf, std::byte{ 0 });
			return false;
		}
		offset += ret;
		size_left -= ret;
	}

	if(vol.FadeVolumeLinear() != 1.0f) {
		const auto apply_volume = (
			(pcmf.format == PCM_SAMPLE_FORMAT::S16) ? ApplyVolume<int16_t> :
			(pcmf.format == PCM_SAMPLE_FORMAT::S32) ? ApplyVolume<int32_t> :
			ApplyVolumeError
		);
		apply_volume(buf, pcmf.channels, vol);
	}
	return true;
}

void TRACK::FadeOut(float volume_start, std::chrono::milliseconds duration)
{
	const auto sample_count = ((duration.count() * pcmf.samplingrate) / 1000);
	vol.SetVolumeLinear(volume_start);
	vol.fade_delta = (volume_start - vol.fade_end);
	vol.fade_end = 0.0f;
	vol.fade_duration = sample_count;
	vol.fade_remaining = sample_count;
}

size_t TRACK_PCM::DecodeSingle(std::span<std::byte> buf)
{
	const auto ret = cur->PartDecodeSingle(buf);
	if(ret == 0) {
		if((cur == intro_part.get()) && (loop_part.get() != nullptr)) {
			cur = loop_part.get();
		}
		cur->PartSeekToSample(0);
	}
	return ret;
}

// Codecs
// ------

struct CODEC_PCM {
	std::u8string_view ext;
	PCM_PART_OPEN& open;
};

std::unique_ptr<PCM_PART> FLAC_Open(
	FILE_STREAM_READ& stream, std::optional<METADATA_CALLBACK> on_metadata
);
std::unique_ptr<PCM_PART> Vorbis_Open(
	FILE_STREAM_READ& stream, std::optional<METADATA_CALLBACK> on_metadata
);

// Sorted in order of preference.
static constexpr const CODEC_PCM CODECS_PCM[] = {
	{ u8".flac", FLAC_Open },
	{ u8".ogg", Vorbis_Open },
};
// ------

static constexpr std::u8string_view LOOP_INFIX = u8".loop";
static constexpr size_t EXT_CAP = std::ranges::max_element(
	CODECS_PCM, [](const auto& a, const auto& b) {
		return (a.ext.size() < b.ext.size());
	}
)->ext.size();

static bool TagEquals(const std::u8string_view a, const std::u8string_view b)
{
	return std::ranges::equal(a, b, [](char a, char b) {
		return (toupper(a) == toupper(b));
	});
}

std::unique_ptr<TRACK> TrackOpen(const std::u8string_view base_fn)
{
	const size_t base_len = base_fn.size();
	const size_t fn_len = (LOOP_INFIX.size() + EXT_CAP);
	std::u8string fn;
	fn.resize_and_overwrite((base_len + fn_len), [&](char8_t* buf, size_t len) {
		std::ranges::copy(base_fn, buf);
		return base_len;
	});
	for(const auto& codec : CODECS_PCM) {
		fn.resize(base_len);
		fn += codec.ext;
		auto intro_stream = FileStreamRead(fn.c_str());
		if(!intro_stream) {
			continue;
		}

		std::unique_ptr<PCM_PART> intro_part;
		std::unique_ptr<PCM_PART> loop_part;
		TRACK_METADATA meta;

		intro_part = codec.open(
			*(intro_stream.get()),
			[&](auto tag, auto value) {
				if(meta.title.empty() && TagEquals(tag, u8"TITLE")) {
					meta.title = value;
					return;
				}
				if(!meta.source_midi && TagEquals(tag, u8"SOURCE MIDI")) {
					meta.source_midi = HashFrom(value);
					return;
				}
				if(!meta.gain_factor && TagEquals(tag, u8"GAIN FACTOR")) {
					const auto first = Narrow::string_view(value).data();
					const auto last = (first + value.size());

#if(__cpp_lib_to_chars >= 201611L)
					float parsed = 0;
					const auto r = std::from_chars(first, last, parsed);
					const auto success = (r.ptr == last);
#else
					// Locale braindeath time!
					// (https://github.com/mpv-player/mpv/commit/1e70e82baa)
					static locale_t locale_c = nullptr;
					if(!locale_c) {
						locale_c = newlocale(LC_NUMERIC_MASK, "C", nullptr);
						if(locale_c) {
							atexit([] {
								freelocale(locale_c);
								locale_c = nullptr;
							});
						}
					}

					char *parsed_last = nullptr;
					float parsed = strtof_l(first, &parsed_last, locale_c);
					const auto success = (parsed_last == last);
#endif
					if(success) {
						meta.gain_factor = parsed;
					}
					return;
				}
			}
		);
		if(!intro_part) {
			continue;
		}

		fn.resize(base_len);
		fn += LOOP_INFIX;
		fn += codec.ext;
		auto loop_stream = FileStreamRead(fn.c_str());
		if(loop_stream) {
			loop_part = codec.open(*(loop_stream.get()), std::nullopt);
			if(!loop_part) {
				continue;
			}
			if(intro_part->pcmf != loop_part->pcmf) {
				assert(!"PCM format mismatch between intro and loop parts!");
				continue;
			}
		}
		return std::make_unique<TRACK_PCM>(
			std::move(meta),
			std::move(intro_stream),
			std::move(loop_stream),
			std::move(intro_part),
			std::move(loop_part)
		);
	}
	return nullptr;
}

} // namespace BGM
