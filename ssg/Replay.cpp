/*
 *   Replay functions
 *
 */

#include "ssg/Replay.h"
#include "ssg/Gian.h"
#include "ssg/internal/LZ.hpp"
#include "hatoyama/logic/buffer.h"
#include "hatoyama/logic/endian.h"

namespace Replay {
#define PREFIX_JA u8"秋霜りぷ"

constexpr std::u8string_view PREFIX_TESTS[] = {
	PREFIX_JA,

	// CP1252 (Western European) (without the initial invalid UTF-8 codepoint)
	u8"‘š‚è‚Õ",

	// Western Windows 9x
	u8"____",
};

constexpr std::u8string_view OLD_BASENAMES[STAGE_MAX] = {
	(PREFIX_JA u8"1.DAT"),
	(PREFIX_JA u8"2.DAT"),
	(PREFIX_JA u8"3.DAT"),
	(PREFIX_JA u8"4.DAT"),
	(PREFIX_JA u8"5.DAT"),
	(PREFIX_JA u8"6.DAT"),
};

std::u8string_view OldBasenameFor(uint8_t stage)
{
	if(stage == STAGE_EXTRA) {
		return (PREFIX_JA u8"Ex.DAT");
	} else if((stage < 1) || (stage > STAGE_MAX)) {
		return { nullptr, 0 };
	}
	return OLD_BASENAMES[stage - 1];
}

uint8_t OldStageNumDetect(std::u8string_view fn)
{
	// Chop off the directory part to avoid false positives within the entirety
	// of the path.
	// `std::string_view::find_last_of()` pulls in way too many C runtime
	// functions on MSVC, so...
	const auto basename_start = ([&] {
		for(size_t i = fn.size(); i >= 1; i--) {
			if((fn[i - 1] == u8'/') || (fn[i - 1] == u8'\\')) {
				return i;
			}
		}
		return 0zu;
	})();
	auto basename = fn;
	basename.remove_prefix(basename_start);

	for(const auto test : PREFIX_TESTS) {
		// Same for `find()`.
		auto haystack = basename;
		while(haystack.size() >= test.size()) {
			if(haystack.starts_with(test)) {
				break;
			}
			haystack.remove_prefix(1);
		}
		if(haystack.size() < test.size()) {
			continue;
		}
		haystack.remove_prefix(test.size());

		// Check stages 1-6
		if(haystack.size() < 1) {
			continue;
		}
		const auto stage_main_i = (haystack[0] - '0');
		if((stage_main_i >= 1) && (stage_main_i <= STAGE_MAX)) {
			return stage_main_i;
		}

		// Check Extra Stage, case-insensitively
		if(haystack.size() < 2) {
			continue;
		}
		if(((haystack[0] | 0x20) == 'e') && ((haystack[1] | 0x20) == 'x')) {
			return STAGE_EXTRA;
		}
	}
	return 0;
}

REPLAY_OLD OldLoad(BUFFER_BORROWED packfile_buf)
{
	const auto in = FilStartR(packfile_buf);

	// ヘッダの格納先は０番である //
	BUFFER_OWNED info_buf = in.MemExpand(0);
	if((nullptr == info_buf) || (info_buf.size() != sizeof(DEMOPLAY_INFO))) {
		return {};
	}
	auto *info = std::bit_cast<DEMOPLAY_INFO *>(info_buf.release());

	// Flip endianness if needed
	info->RndSeed = LEAt(&info->RndSeed);
	info->FrameCount = LEAt(&info->FrameCount);

	// データの格納先は１番ですね //
	BUFFER_OWNED frames_buf = in.MemExpand(1);
	const auto frames_buf_size_min = (sizeof(INPUT_BITS) * info->FrameCount);
	if((nullptr == frames_buf) || (frames_buf.size() < frames_buf_size_min)) {
		return {};
	}

	// Flip endianness if needed. The optimizer will remove this entire loop on
	// little-endian systems!
	auto *frames_ptr = std::bit_cast<INPUT_BITS *>(frames_buf.release());
	for(const auto i : std::views::iota(0u, info->FrameCount)) {
		frames_ptr[i] = LEAt(&frames_ptr[i]);
	}

	return REPLAY_OLD{
		.Info = { info, std::move(info_buf.get_deleter()) },
		.Frames = { frames_ptr, std::move(frames_buf.get_deleter()) },
		.FramesStored = frames_buf.size(),
	};
}

} // namespace Replay
