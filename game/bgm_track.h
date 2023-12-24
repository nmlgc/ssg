/*
 *   BGM tracks that are rendered as PCM and output via the Snd subsystem
 *
 *   Adapted from thcrap's bgmmod module.
 */

#pragma once

#include "game/pcm.h"
#include "platform/file.h"
#include <stdint.h>
#include <memory>
#include <span>

namespace BGM {

// Base class for a track
// ----------------------

struct TRACK {
	// Target format that this track is decoded to.
	const PCM_FORMAT pcmf;

	// Single decoding call that implicitly handles looping. Should return the
	// number of bytes actually decoded (which can be less than
	// [buf.size_bytes()]), or -1 if an error occurred.
	virtual size_t DecodeSingle(std::span<std::byte> buf) = 0;

	// *Always* fills [buf] entirely. Returns `true` if successful, or `false`
	// in case of an unrecoverable decoding error, in which case [buf] is
	// filled with zeroes.
	bool Decode(std::span<std::byte> buf);

	TRACK(const PCM_FORMAT& pcmf) : pcmf(pcmf) {
	}

	virtual ~TRACK() {}
};
// ----------------------

// Tracks in PCM source formats
// ----------------------------

// Base class for an individual intro or loop file.
// Should be derived for each supported codec.
struct PCM_PART {
	const PCM_FORMAT pcmf;

	// Single decoding call. Should return the number of bytes actually decoded
	// (which can be less than [buf.size_bytes()]) or -1 if an error occurred.
	virtual size_t PartDecodeSingle(std::span<std::byte> buf) = 0;

	// Seeks to the given raw decoded audio sample. Guaranteed to be less than
	// the total number of samples in the stream.
	virtual void PartSeekToSample(size_t sample) = 0;

	PCM_PART(const PCM_FORMAT& pcmf) : pcmf(pcmf) {
	}
	virtual ~PCM_PART() {}
};

// Generic implementation for PCM-based codecs, with separate intro and loop
// files.
struct TRACK_PCM : public TRACK {
	std::unique_ptr<FILE_STREAM_READ> intro_stream;
	std::unique_ptr<FILE_STREAM_READ> loop_stream;
	std::unique_ptr<PCM_PART> intro_part;
	std::unique_ptr<PCM_PART> loop_part;
	PCM_PART* cur;

	virtual size_t DecodeSingle(std::span<std::byte> buf) override;

	TRACK_PCM(
		std::unique_ptr<FILE_STREAM_READ> intro_stream,
		std::unique_ptr<FILE_STREAM_READ> loop_stream,
		std::unique_ptr<PCM_PART> intro_part,
		std::unique_ptr<PCM_PART> loop_part
	) :
		TRACK(intro_part->pcmf),
		intro_stream(std::move(intro_stream)),
		loop_stream(std::move(loop_stream)),
		intro_part(std::move(intro_part)),
		loop_part(std::move(loop_part)),
		cur(this->intro_part.get())
	{
	}

	virtual ~TRACK_PCM() {}
};

// Tries to opens [stream] as a part of a modded track, using a specific codec.
// `TRACK_PCM` retains ownership of [stream].
using PCM_PART_OPEN = std::unique_ptr<PCM_PART>(FILE_STREAM_READ& stream);
// ----------------------------

// Tries to open a waveform track whose name starts with [base_fn] and has one
// of the supported codec extensions.
std::unique_ptr<TRACK> TrackOpen(const std::u8string_view base_fn);

} // namespace BGM
