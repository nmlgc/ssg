/*                                                                           */
/*   DemoPlay.cpp   デモプレイ処理                                           */
/*                                                                           */
/*                                                                           */

#include <SDL3/SDL_iostream.h>

#include "DEMOPLAY.H"
#include "LOADER.H"
#include "LZ_UTY.H"
#include "LogicInstance.h"
#include "ssg/API.h"
#include "ssg/Replay.h"
#include "ssg/internal/LZ.hpp"
#include "ssg/internal/Maid.hpp"
#include "ssg/internal/RNG.hpp"
#include "ssg/internal/Stage.hpp"
#include "hatoyama/app/file.h"
#include "hatoyama/logic/endian.h"
#include "hatoyama/logic/ut_math.h"


bool	DemoplaySaveEnable = false;	// デモプレイのセーブが動作しているか
uint32_t	DemoplayLoadFrameCount = 0;	// > 0 == replay loaded
DEMOPLAY_INFO	DemoSaveInfo;
static uint32_t DemoFrameCur;

// デモプレイ用バッファ
ENDIAN_LITTLE<INPUT_BITS> DemoBuffer[DEMOBUF_MAX];


bool DemoplayIsPlaying(void)
{
	return (DemoplayLoadFrameCount > 0);
}

void DemoplayInit(const ROUND_PARAMS& Round)
{
	// 乱数の準備 //
	DemoSaveInfo.RndSeed = Round.Seed;
	DemoSaveInfo.Exp = Round.Exp;
	DemoSaveInfo.Weapon = Round.Weapon;
	DemoSaveInfo.CfgDat.LevelSelected = Round.LevelSelected;
	DemoSaveInfo.CfgDat.PlayerStock = Round.PlayerStock;
	DemoSaveInfo.CfgDat.BombStock = Round.BombStock;
	DemoSaveInfo.CfgDat.InputFlags = Round.InputFlags;

	DemoFrameCur = 0;
	DemoplaySaveEnable = true;
}


bool DemoplayLoadSetup(const DEMOPLAY_INFO& info, uint8_t stage)
{
	DemoFrameCur = 0;
	DemoplayLoadFrameCount = info.FrameCount;
	SSG.RoundInitFromReplayOld(info, stage);
	return true;
}


bool DemoplayRecord(INPUT_BITS key)
{
	if(!DemoplaySaveEnable) {
		return false;
	}

	DemoBuffer[DemoFrameCur++] = key;

	// バッファが最後に来たか、ＥＳＣが押された場合 //
	if((DemoFrameCur == DEMOBUF_MAX) || (key & KEY_ESC)) {
		DemoFrameCur--;
		return true;
	}
	return false;
}


void DemoplaySaveDemo(void)
{
	if(!DemoplaySaveEnable) return;

	DemoBuffer[DemoFrameCur] = KEY_ESC;
	DemoSaveInfo.FrameCount = (DemoFrameCur + 1);

	char8_t fn[] = u8"STG_Demo.DAT";
	fn[3] = ('0' + SSG.Stage.Number());

	auto *f = SDL_IOFromFile(fn, "wb");
	if(f) {
		SDL_WriteIO(f, &DemoSaveInfo, sizeof(DemoSaveInfo));
		SDL_WriteIO(
			f, DemoBuffer, (sizeof(DemoBuffer[0]) * DemoSaveInfo.FrameCount)
		);
		SDL_CloseIO(f);
	}

	DemoplaySaveEnable = false;
}


bool DemoplayLoadDemo(uint8_t stage)
{
	// 展開 //
	const auto temp = LoadDemo(stage);
	auto cursor = temp.cursor();

	const auto maybe_info = cursor.next<DEMOPLAY_INFO>();
	if(!maybe_info) {
		return false;
	}
	const auto& info = maybe_info.value()[0];
	{
		const auto maybe_inputs = cursor.next<ENDIAN_LITTLE<INPUT_BITS>>(
			info.FrameCount
		);
		if(!maybe_inputs) {
			return false;
		}
		const auto inputs = maybe_inputs.value();
		std::ranges::copy(inputs, DemoBuffer);
	}
	return DemoplayLoadSetup(info, stage);
}


INPUT_BITS DemoplayMove(void)
{
	if(!DemoplayIsPlaying()) {
		return KEY_ESC;
	}

	const auto ptr = DemoFrameCur;
	if(ptr >= DemoplayLoadFrameCount) {
		DemoplayCleanup();
	}
	else DemoFrameCur++;

	return DemoBuffer[ptr];
}


void DemoplayCleanup(void)
{
	DemoplayLoadFrameCount = 0;
}


void DemoplaySaveReplay(void)
{
	if(!DemoplaySaveEnable) return;

	// すぐさま、無効化する //
	DemoplaySaveEnable = false;

	DemoBuffer[DemoFrameCur] = KEY_ESC;
	DemoSaveInfo.FrameCount = (DemoFrameCur + 1);

	static_assert(std::is_same_v<
		decltype(DemoBuffer[0]), ENDIAN_LITTLE<INPUT_BITS>&
	>);
	BUFFER_BORROWED files[] = {
		std::span(&DemoSaveInfo, 1),
		std::span(DemoBuffer, DemoSaveInfo.FrameCount),
	};

	const auto *fn = ReplayOldBasenameFor(SSG.Stage.Number());
	FilWrite(fn, files);
}


bool DemoplayLoadReplay(uint8_t stage)
{
	const auto *fn = ReplayOldBasenameFor(stage);
	const auto fil = SDL_LoadFile(fn);
	auto replay = Replay::OldLoad(BUFFER_BORROWED{ fil });
	if(!replay) {
		return false;
	}
	auto& info = *replay.Info;
	std::ranges::copy_n(replay.Frames.get(), info.FrameCount, DemoBuffer);

	// Repair any replay-related bugs from earlier builds
	// --------------------------------------------------

	bool repair = false;
	const auto file_level = info.CfgDat.LevelSelected;
	const auto file_lives = info.CfgDat.PlayerStock;

	// ≥P0295 was writing the configured difficulty and starting lives for the
	// regular 6 stages instead of the fixed values for the Extra Stage.
	// MODDERS: If this bug doesn't apply to you because you modded an earlier
	// build, mark the affected files as read-only to skip this patch.
	if(
		(stage == STAGE_EXTRA) &&
		((file_level != EXTRA_LEVEL) || (file_lives != EXTRA_LIVES))
	) {
		info.CfgDat.LevelSelected = EXTRA_LEVEL;
		info.CfgDat.PlayerStock = EXTRA_LIVES;
		repair = true;
	}

	// ≥P0217 was writing twice as many frames as recorded.
	if(replay.FramesStored != (sizeof(INPUT_BITS) * info.FrameCount)) {
		repair = true;
	}

	if(repair) {
		static_assert(std::is_same_v<
			decltype(DemoBuffer[0]), ENDIAN_LITTLE<INPUT_BITS>&
		>);
		BUFFER_BORROWED files[] = {
			std::span(&info, 1),
			std::span(DemoBuffer, info.FrameCount),
		};
		if(!FilWrite(fn, files, File_TimestampsGet(fn))) {
			// Repair denied by file being read-only? OK, bro, you're the boss!
			info.CfgDat.LevelSelected = file_level;
			info.CfgDat.PlayerStock = file_lives;
		}
	}
	// --------------------------------------------------
	return DemoplayLoadSetup(info, stage);
}
