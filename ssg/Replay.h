/*
 *   Replay functions
 *
 */

#pragma once

#include "hatoyama/api/export.h"
#include "ssg/Input.h"

#ifdef __cplusplus
#include "hatoyama/logic/buffer.h"
#else
#include <stdint.h>
#endif

///// Replay-specific config option subset /////
// The original code simply reused `CONFIG_DATA`, which we can't do in this
// fork since has has long ceased being logic-exclusive with all the fields
// we have added to the structure.
typedef struct {
	uint8_t LevelSelected;
	uint8_t PlayerStock;
	uint8_t BombStock;
	uint8_t Padding1[5];
	uint8_t InputFlags;
	uint8_t Padding2[15];
} DEMOPLAY_CONFIG_DATA;

// On-disk header "file" of the one-stage format
typedef struct {
	// 乱数のたね
	uint32_t RndSeed;

	// Not data size! Including the terminating ESC.
	uint32_t FrameCount;

	// コンフィグの情報(Load時に一部を参照する)
	DEMOPLAY_CONFIG_DATA CfgDat;

	// 初期パワーアップ
	uint8_t Exp;

	// 初期装備
	uint8_t Weapon;
} DEMOPLAY_INFO;

// FFI structure containing raw, non-owning pointers into a `REPLAY_OLD`.
typedef struct {
	DEMOPLAY_INFO *Info;
	INPUT_BITS *Frames; // [Info->FrameCount] elements
} REPLAY_OLD_PTRS;

#ifdef __cplusplus
struct REPLAY_OLD {
	std::unique_ptr<DEMOPLAY_INFO, BUFFER_DELETER> Info;
	std::unique_ptr<INPUT_BITS[], BUFFER_DELETER> Frames;
	size_t FramesStored = 0; // May differ from [Info->FrameCount].

	explicit operator bool() const {
		return (Info && Frames);
	}
};

namespace Replay {
// Returns the game's original expected filename of the single one-stage replay
// for the given [stage] in Shuusou Gyoku's original format, or an empty view
// for invalid [stage] numbers.
HATOYAMA_API std::u8string_view OldBasenameFor(uint8_t stage);

// Detects which stage the given one-stage replay filename corresponds to, by
// comparing the filename portion of [fn] against several encodings of the
// original Japanese replay file name prefix. Returns 0 if detection failed.
HATOYAMA_API uint8_t OldStageNumDetect(std::u8string_view fn);

// Loads a one-stage replay from a compressed buffer. Returns an empty
// `REPLAY_OLD` that boolean-compares to `false` if [packfile_buf] is not a
// valid old-format replay, or if out of memory.
HATOYAMA_API REPLAY_OLD OldLoad(BUFFER_BORROWED packfile_buf);
} // namespace Replay
#endif
