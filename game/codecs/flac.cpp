/*
 *   FLAC streaming support
 *
 *   Adapted from thcrap's bgmmod module.
 */

#define DR_FLAC_IMPLEMENTATION
#define DR_FLAC_NO_STDIO

// GCC 15 throws `error: conflicting declaration 'typedef struct max_align_t
// max_align_t'` if this appears after a module import.
#include <SDL3/SDL_iostream.h>
#include <libs/dr_libs/dr_flac.h>

#include "game/bgm_track.h"

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
	{ t.stream() } -> std::same_as<SDL_IOStream&>;
};

struct CB_DATA_STREAM {
	SDL_IOStream& stream() {
		return *std::bit_cast<SDL_IOStream *>(this);
	}
};

struct CB_DATA_STREAM_AND_METADATA {
	SDL_IOStream& _stream;
	METADATA_CALLBACK on_metadata;

	SDL_IOStream& stream() {
		return _stream;
	}
};

template <CB_DATA CB> static size_t CB_FLAC_Read(
	void* user_data, void* buf, size_t size
)
{
	auto& stream = static_cast<CB *>(user_data)->stream();
	return SDL_ReadIO(&stream, buf, size);
}

template <CB_DATA CB> static drflac_bool32 CB_FLAC_Seek(
	void* user_data, int offset, drflac_seek_origin origin
)
{
	auto& stream = static_cast<CB *>(user_data)->stream();
	static_assert(
		std::to_underlying(DRFLAC_SEEK_SET) ==
		std::to_underlying(SDL_IO_SEEK_SET)
	);
	static_assert(
		std::to_underlying(DRFLAC_SEEK_CUR) ==
		std::to_underlying(SDL_IO_SEEK_CUR)
	);
	static_assert(
		std::to_underlying(DRFLAC_SEEK_END) ==
		std::to_underlying(SDL_IO_SEEK_END)
	);
	const auto whence = static_cast<SDL_IOWhence>(origin);
	return (SDL_SeekIO(&stream, offset, whence) != -1);
}

template <CB_DATA CB> static drflac_bool32 CB_FLAC_Tell(
	void* user_data, drflac_int64 *cursor
)
{
	auto& stream = static_cast<CB *>(user_data)->stream();
	const auto offset = SDL_TellIO(&stream);
	if(!cursor || (offset == -1)) {
		return DRFLAC_FALSE;
	}
	*cursor = offset;
	return DRFLAC_TRUE;
}

#pragma warning(suppress: 26461) // con.3
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
	SDL_IOStream& stream, std::optional<METADATA_CALLBACK> on_metadata
)
{
	if(const auto& metadata_cb = on_metadata) {
		const auto initial_offset = SDL_TellIO(&stream);
		if(initial_offset == -1) {
			return nullptr;
		}
		CB_DATA_STREAM_AND_METADATA data = { stream, *metadata_cb };
		auto* ff = drflac_open_with_metadata(
			CB_FLAC_Read<CB_DATA_STREAM_AND_METADATA>,
			CB_FLAC_Seek<CB_DATA_STREAM_AND_METADATA>,
			CB_FLAC_Tell<CB_DATA_STREAM_AND_METADATA>,
			CB_FLAC_Meta,
			&data,
			nullptr
		);
		if(ff) {
			drflac_close(ff);
		}
		const auto new_offset = SDL_SeekIO(
			&stream, initial_offset, SDL_IO_SEEK_SET
		);
		if(new_offset != initial_offset) {
			return nullptr;
		}
	}
	auto* ff = drflac_open(
		CB_FLAC_Read<CB_DATA_STREAM>,
		CB_FLAC_Seek<CB_DATA_STREAM>,
		CB_FLAC_Tell<CB_DATA_STREAM>,
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
