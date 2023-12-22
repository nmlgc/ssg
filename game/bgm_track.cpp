/*
 *   BGM tracks that are rendered as PCM and output via the Snd subsystem
 *
 *   Adapted from thcrap's bgmmod module.
 */

#include "game/bgm_track.h"
#include <assert.h>
#include <algorithm>

namespace BGM {

void OnVorbisComment(METADATA_CALLBACK func, const std::u8string_view comment)
{
	const auto eq_i = comment.find('=');
	if(eq_i == std::u8string_view::npos) {
		return;
	}
	func(comment.substr(0, eq_i), comment.substr(eq_i + 1));
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
	return true;
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
