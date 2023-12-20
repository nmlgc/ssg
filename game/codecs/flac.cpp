/*
 *   FLAC streaming support
 *
 *   Adapted from thcrap's bgmmod module.
 */

#include "game/bgm_track.h"

#define DR_FLAC_IMPLEMENTATION
#define DR_FLAC_NO_STDIO
#include <libs/dr_libs/dr_flac.h>

namespace BGM {

// Callbacks
// ---------

static size_t CB_FLAC_Read(void* user_data, void* buf, size_t size)
{
	auto& stream = *static_cast<FILE_STREAM_READ *>(user_data);
	return stream.Read({ static_cast<uint8_t *>(buf), size });
}

static drflac_bool32 CB_FLAC_Seek(
	void* user_data, int offset, drflac_seek_origin origin
)
{
	auto& stream = *static_cast<FILE_STREAM_READ *>(user_data);
	const auto whence = ((origin == drflac_seek_origin_start)
		? SEEK_WHENCE::BEGIN
		: SEEK_WHENCE::CURRENT
	);
	return stream.Seek(offset, whence);
}
// ---------

typedef drflac_uint64 drflac_read_func_t(drflac *, drflac_uint64, void *);

struct PCM_PART_FLAC : public PCM_PART {
	drflac* ff;
	drflac_read_func_t& read_func;

	size_t PartDecodeSingle(std::span<std::byte> buf) override;
	void PartSeekToSample(size_t sample) override;

	PCM_PART_FLAC(
		drflac* ff, const PCM_FORMAT& pcmf, drflac_read_func_t& read_func
	) : PCM_PART(pcmf), ff(ff), read_func(read_func) {
	}
	virtual ~PCM_PART_FLAC();
};

size_t PCM_PART_FLAC::PartDecodeSingle(std::span<std::byte> buf)
{
	const auto sample_size = pcmf.SampleSize();
	const auto samples = (buf.size_bytes() / sample_size);
	return (read_func(ff, samples, buf.data()) * sample_size);
}

void PCM_PART_FLAC::PartSeekToSample(size_t sample)
{
	drflac_seek_to_pcm_frame(ff, sample);
}

PCM_PART_FLAC::~PCM_PART_FLAC()
{
	drflac_close(ff);
}

std::unique_ptr<PCM_PART> FLAC_Open(FILE_STREAM_READ& stream)
{
	auto* ff = drflac_open(CB_FLAC_Read, CB_FLAC_Seek, &stream, nullptr);
	if(!ff) {
		return nullptr;
	}
	const auto output_format = ((ff->bitsPerSample <= 16)
		? PCM_SAMPLE_FORMAT::S16
		: PCM_SAMPLE_FORMAT::S32
	);
	const auto read_func = ((ff->bitsPerSample <= 16)
		? reinterpret_cast<drflac_read_func_t &>(drflac_read_pcm_frames_s16)
		: reinterpret_cast<drflac_read_func_t &>(drflac_read_pcm_frames_s32)
	);
	PCM_FORMAT pcmf = { ff->sampleRate, ff->channels, output_format };
	return std::make_unique<PCM_PART_FLAC>(ff, pcmf, *read_func);
}

} // namespace BGM
