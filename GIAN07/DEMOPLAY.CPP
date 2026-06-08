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
#include "ssg/Input.h"
#include "ssg/internal/LZ.hpp"
#include "ssg/internal/Maid.hpp"
#include "ssg/internal/RNG.hpp"
#include "ssg/internal/Stage.hpp"
#include "hatoyama/app/file.h"
#include "hatoyama/logic/ut_math.h"


bool	DemoplaySaveEnable = false;	// デモプレイのセーブが動作しているか
bool	DemoplayLoadEnable = false;	// デモプレイのロードが動作しているか
DEMOPLAY_INFO	DemoInfo;						// デモプレイ情報
INPUT_BITS	DemoBuffer[DEMOBUF_MAX];	// デモプレイ用バッファ
static uint32_t DemoFrameCur;


void DemoplayInit(const ROUND_PARAMS& Round)
{
	// 乱数の準備 //
	DemoInfo.RndSeed = Round.Seed;
	DemoInfo.Exp = Round.Exp;
	DemoInfo.Weapon = Round.Weapon;
	DemoInfo.CfgDat.LevelSelected = Round.LevelSelected;
	DemoInfo.CfgDat.PlayerStock = Round.PlayerStock;
	DemoInfo.CfgDat.BombStock = Round.BombStock;
	DemoInfo.CfgDat.InputFlags = Round.InputFlags;

	DemoFrameCur = 0;
	DemoplaySaveEnable = true;
}


bool DemoplayLoadSetup(uint8_t stage)
{
	DemoFrameCur = 0;
	DemoplayLoadEnable = true;

	const ROUND_PARAMS round = {
		.Seed = DemoInfo.RndSeed,

		// コンフィグの初期化 //
		.LevelSelected = DemoInfo.CfgDat.LevelSelected,
		.PlayerStock = DemoInfo.CfgDat.PlayerStock,
		.BombStock = DemoInfo.CfgDat.BombStock,
		.InputFlags = DemoInfo.CfgDat.InputFlags,

		// 本体の性能記述 //
		.Weapon = DemoInfo.Weapon,
		.Exp = DemoInfo.Exp,

		.MsgEnabled = false,
	};
	SSG.RoundInit(round, stage);

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
	DemoInfo.FrameCount = (DemoFrameCur + 1);

	char8_t fn[] = u8"STG_Demo.DAT";
	fn[3] = ('0' + SSG.Stage.Number());

	auto *f = SDL_IOFromFile(fn, "wb");
	if(f) {
		SDL_WriteIO(f, &DemoInfo, sizeof(DemoInfo));
		SDL_WriteIO(
			f, DemoBuffer, (sizeof(DemoBuffer[0]) * DemoInfo.FrameCount)
		);
		SDL_CloseIO(f);
	}

	DemoplaySaveEnable = false;
}


bool DemoplayLoadDemo(uint8_t stage)
{
	// 展開 //
	const auto temp = LoadDemo(stage);
	auto temp_cursor = temp.cursor();
	{
		const auto maybe_info = temp_cursor.next<DEMOPLAY_INFO>();
		if(!maybe_info) {
			return false;
		}
		DemoInfo = maybe_info.value()[0];
	}
	{
		const auto maybe_inputs = temp_cursor.next<uint16_t>(DemoInfo.FrameCount);
		if(!maybe_inputs) {
			return false;
		}
		const auto inputs = maybe_inputs.value();
		memcpy(DemoBuffer, inputs.data(), inputs.size_bytes());
	}
	return DemoplayLoadSetup(stage);
}


INPUT_BITS DemoplayMove(void)
{
	if(!DemoplayLoadEnable) return KEY_ESC;

	const auto ptr = DemoFrameCur;
	if(ptr>=DemoInfo.FrameCount){
		DemoplayLoadEnable = false;
	}
	else DemoFrameCur++;

	return DemoBuffer[ptr];
}


void DemoplayCleanup(void)
{
	DemoplayLoadEnable = false;
}


void DemoplaySaveReplay(void)
{
	if(!DemoplaySaveEnable) return;

	// すぐさま、無効化する //
	DemoplaySaveEnable = false;

	DemoBuffer[DemoFrameCur] = KEY_ESC;
	DemoInfo.FrameCount = (DemoFrameCur + 1);

	BUFFER_BORROWED files[] = {
		std::span(&DemoInfo, 1), std::span(DemoBuffer, DemoInfo.FrameCount),
	};

	const auto *fn = ReplayOldBasenameFor(SSG.Stage.Number());
	FilWrite(fn, files);
}


bool DemoplayLoadReplay(uint8_t stage)
{
	BUFFER_OWNED temp;

	const auto *fn = ReplayOldBasenameFor(stage);
	const auto fil = SDL_LoadFile(fn);
	const auto in = FilStartR(BUFFER_BORROWED{ fil });

	// ヘッダの格納先は０番である //
	temp = in.MemExpand( 0);
	if(nullptr == temp) {
		return false;
	}
	memcpy(&DemoInfo, temp.get(), sizeof(DEMOPLAY_INFO));

	// データの格納先は１番ですね //
	temp = in.MemExpand( 1);
	if(nullptr == temp) {
		return false;
	}
	memcpy(DemoBuffer, temp.get(), (sizeof(INPUT_BITS) * DemoInfo.FrameCount));

	// Repair any replay-related bugs from earlier builds
	// --------------------------------------------------

	bool repair = false;
	const auto file_level = DemoInfo.CfgDat.LevelSelected;
	const auto file_lives = DemoInfo.CfgDat.PlayerStock;

	// ≥P0295 was writing the configured difficulty and starting lives for the
	// regular 6 stages instead of the fixed values for the Extra Stage.
	// MODDERS: If this bug doesn't apply to you because you modded an earlier
	// build, mark the affected files as read-only to skip this patch.
	if(
		(stage == STAGE_EXTRA) &&
		((file_level != EXTRA_LEVEL) || (file_lives != EXTRA_LIVES))
	) {
		DemoInfo.CfgDat.LevelSelected = EXTRA_LEVEL;
		DemoInfo.CfgDat.PlayerStock = EXTRA_LIVES;
		repair = true;
	}

	// ≥P0217 was writing twice as many frames as recorded.
	if(temp.size() != (sizeof(INPUT_BITS) * DemoInfo.FrameCount)) {
		repair = true;
	}

	if(repair) {
		BUFFER_BORROWED files[] = {
			std::span(&DemoInfo, 1),
			std::span(DemoBuffer, DemoInfo.FrameCount),
		};
		if(!FilWrite(fn, files, File_TimestampsGet(fn))) {
			// Repair denied by file being read-only? OK, bro, you're the boss!
			DemoInfo.CfgDat.LevelSelected = file_level;
			DemoInfo.CfgDat.PlayerStock = file_lives;
		}
	}
	// --------------------------------------------------
	return DemoplayLoadSetup(stage);
}
