/*
 *   Replay functions
 *
 */

#include "ssg/Replay.h"
#include "ssg/Gian.h"

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

} // namespace Replay
