/*
 *   Vorbis streaming support
 *
 *   Adapted from thcrap's bgmmod module.
 */

#include "game/bgm_track.h"
#include <assert.h>

#define OV_EXCLUDE_STATIC_CALLBACKS
#include <vorbis/vorbisfile.h>

namespace BGM {

// Callbacks
// ---------

static size_t CB_Vorbis_Read(
	void* ptr, size_t size, size_t nmemb, void* datasource
)
{
	auto* stream = static_cast<FILE_STREAM_READ *>(datasource);
	return stream->Read({ static_cast<uint8_t *>(ptr), (size * nmemb) });
}

int CB_Vorbis_Seek(void* datasource, ogg_int64_t offset, int ov_whence)
{
	auto* stream = static_cast<FILE_STREAM_READ *>(datasource);
	
	#pragma warning(suppress : 26494) // type.5
	SEEK_WHENCE whence;
	switch(ov_whence) {
	case 0:	whence = SEEK_WHENCE::BEGIN;  	break;
	case 1:	whence = SEEK_WHENCE::CURRENT;	break;
	case 2:	whence = SEEK_WHENCE::END;    	break;
	default:
		assert(!"Invalid seek origin?");
		return -1;
	}
	return stream->Seek(offset, whence);
}

long CB_Vorbis_Tell(void* datasource)
{
	auto* stream = static_cast<FILE_STREAM_READ *>(datasource);
	return stream->Tell().value_or(-1);
}

static const ov_callbacks VORBIS_CALLBACKS = {
	CB_Vorbis_Read, CB_Vorbis_Seek, nullptr, CB_Vorbis_Tell,
};
// ---------

struct PCM_PART_VORBIS : public BGM::PCM_PART {
	OggVorbis_File vf;

	size_t PartDecodeSingle(std::span<std::byte> buf) override;
	void PartSeekToSample(size_t sample) override;

	PCM_PART_VORBIS(OggVorbis_File&& vf, const PCM_FORMAT& pcmf) :
		vf(vf), PCM_PART(pcmf) {
	}
	virtual ~PCM_PART_VORBIS();
};

size_t PCM_PART_VORBIS::PartDecodeSingle(std::span<std::byte> buf)
{
	assert(pcmf.format == PCM_SAMPLE_FORMAT::S16);
	auto* buf_as_char = reinterpret_cast<char *>(buf.data());
	return ov_read(&vf, buf_as_char, buf.size_bytes(), 0, 2, 1, nullptr);
}

void PCM_PART_VORBIS::PartSeekToSample(size_t sample)
{
	const auto ret = ov_pcm_seek(&vf, sample);
	assert(ret == 0);
}

PCM_PART_VORBIS::~PCM_PART_VORBIS()
{
	ov_clear(&vf);
}

std::unique_ptr<BGM::PCM_PART> Vorbis_Open(
	FILE_STREAM_READ& stream, std::optional<BGM::METADATA_CALLBACK> on_metadata
)
{
	OggVorbis_File vf = { 0 };
	const auto ret = ov_open_callbacks(
		&stream, &vf, nullptr, 0, VORBIS_CALLBACKS
	);
	if(ret || !vf.vi) {
		return nullptr;
	}
	assert(vf.vi->rate >= 0);
	assert(vf.vi->channels >= 0);

	if(on_metadata) {
		const auto* vc = ov_comment(&vf, -1);
		if(vc) {
			for(decltype(vc->comments) i = 0; i < vc->comments; i++) {
				// Why signed!?
				const auto len = vc->comment_lengths[i];
				if(vc->comment_lengths[i] < 2) {
					continue;
				}
				BGM::OnVorbisComment(on_metadata.value(), {
					reinterpret_cast<const char8_t *>(vc->user_comments[i]),
					static_cast<size_t>(len),
				});
			}
		}
	}

	const auto samplingrate = static_cast<uint32_t>(vf.vi->rate);
	const auto channels = static_cast<uint16_t>(vf.vi->channels);
	PCM_FORMAT pcmf = { samplingrate, channels, PCM_SAMPLE_FORMAT::S16 };
	return std::make_unique<PCM_PART_VORBIS>(std::move(vf), pcmf);
}

} // namespace BGM
