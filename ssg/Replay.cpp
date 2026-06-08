/*
 *   Replay functions
 *
 */

#include "ssg/Replay.h"
#include "ssg/Gian.h"

namespace Replay {
#define PREFIX_JA u8"秋霜りぷ"

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
} // namespace Replay
