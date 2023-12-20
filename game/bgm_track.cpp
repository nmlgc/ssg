/*
 *   BGM tracks that are rendered as PCM and output via the Snd subsystem
 *
 *   Adapted from thcrap's bgmmod module.
 */

#include "game/bgm_track.h"
#include <assert.h>
#include <algorithm>

namespace BGM {

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

} // namespace BGM
