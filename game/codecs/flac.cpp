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

// dr_flac's metadata-supporting opening function takes a single [user_data]
// pointer, just like the regular one without metadata support. This assumes
// that both the file stream and the target location for the metadata are
// reachable from the same pointer and share the same lifetime, which doesn't
// match our architecture of centralized metadata parsing. This forces us to
// load the intro file twice (once with metadata, and once without), and employ
// a bit of trickery to avoid duplicating the read and seek callbacks for the
// two cases...
template <class T> concept CB_DATA = requires(T t) {
	{ t.stream() } -> std::same_as<FILE_STREAM_READ &>;
};

struct CB_DATA_STREAM : public FILE_STREAM_READ {
	FILE_STREAM_READ& stream() {
		return *this;
	}
};

struct CB_DATA_STREAM_AND_METADATA {
	FILE_STREAM_READ& _stream;
	METADATA_CALLBACK on_metadata;

	FILE_STREAM_READ& stream() {
		return _stream;
	}
};

template <CB_DATA CB> static size_t CB_FLAC_Read(
	void* user_data, void* buf, size_t size
)
{
	auto& stream = static_cast<CB *>(user_data)->stream();
	return stream.Read({ static_cast<uint8_t *>(buf), size });
}

template <CB_DATA CB> static drflac_bool32 CB_FLAC_Seek(
	void* user_data, int offset, drflac_seek_origin origin
)
{
	auto& stream = static_cast<CB *>(user_data)->stream();
	const auto whence = ((origin == drflac_seek_origin_start)
		? SEEK_WHENCE::BEGIN
		: SEEK_WHENCE::CURRENT
	);
	return stream.Seek(offset, whence);
}

[[gsl::suppress(con.3)]]
static void CB_FLAC_Meta(void* user_data, drflac_metadata* metadata)
{
	if(metadata->type != DRFLAC_METADATA_BLOCK_TYPE_VORBIS_COMMENT) {
		return;
	}
	const auto* cb_data = static_cast<CB_DATA_STREAM_AND_METADATA *>(user_data);

	drflac_vorbis_comment_iterator it;
	drflac_init_vorbis_comment_iterator(
		&it,
		metadata->data.vorbis_comment.commentCount,
		metadata->data.vorbis_comment.pComments
	);
	const char* cmt_str = nullptr;
	drflac_uint32 cmt_len = 0;
	while((cmt_str = drflac_next_vorbis_comment(&it, &cmt_len)) != nullptr) {
		OnVorbisComment(
			cb_data->on_metadata,
			{ reinterpret_cast<const char8_t *>(cmt_str), cmt_len }
		);
	}
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

std::unique_ptr<PCM_PART> FLAC_Open(
	FILE_STREAM_READ& stream, std::optional<METADATA_CALLBACK> on_metadata
)
{
	if(on_metadata) {
		const auto maybe_initial_offset = stream.Tell();
		if(!maybe_initial_offset.has_value()) {
			return nullptr;
		}
		CB_DATA_STREAM_AND_METADATA data = { stream, on_metadata.value() };
		auto* ff = drflac_open_with_metadata(
			CB_FLAC_Read<CB_DATA_STREAM_AND_METADATA>,
			CB_FLAC_Seek<CB_DATA_STREAM_AND_METADATA>,
			CB_FLAC_Meta,
			&data,
			nullptr
		);
		if(ff) {
			drflac_close(ff);
		}
		if(!stream.Seek(maybe_initial_offset.value(), SEEK_WHENCE::BEGIN)) {
			return nullptr;
		}
	}
	auto* ff = drflac_open(
		CB_FLAC_Read<CB_DATA_STREAM>,
		CB_FLAC_Seek<CB_DATA_STREAM>,
		&stream,
		nullptr
	);
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
