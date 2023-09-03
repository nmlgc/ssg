/*                                                                           */
/*   DemoPlay.cpp   デモプレイ処理                                           */
/*                                                                           */
/*                                                                           */

#include "platform/file.h"
#include "LZ_UTY.H"
#include "GIAN.H"
#include "DEMOPLAY.H"
#include "game/input.h"


bool	DemoplaySaveEnable = false;	// デモプレイのセーブが動作しているか
bool	DemoplayLoadEnable = false;	// デモプレイのロードが動作しているか
DEMOPLAY_INFO	DemoInfo;						// デモプレイ情報
INPUT_BITS	DemoBuffer[DEMOBUF_MAX];	// デモプレイ用バッファ
DWORD			DemoBufPtr;						// デモプレイ用バッファPtr(Ｃのそれではない)
CONFIG_DATA		ConfigTemp;						// コンフィグのデータ一時保存用

//DWORD RndBuf[RNDBUF_MAX];



PATH_STRING ReplayFN(uint8_t stage)
{
	if(stage == GRAPH_ID_EXSTAGE) {
		return PATH_STRING{ _PATH("秋霜りぷEx.DAT") };
	}

	auto ret = PATH_STRING{ _PATH("秋霜りぷ_.DAT") };
	ret[ret.size() - 5] = ('0' + GameStage);
	return ret;
}

void DemoplayInit(void)
{
	// 乱数の準備 //
	DemoInfo.RndSeed = (DWORD)(rnd()+1)*(DWORD)(rnd()+1);
	rnd_seed_set(DemoInfo.RndSeed);

	DemoInfo.Exp    = Viv.exp;
	DemoInfo.Weapon = Viv.weapon;
	DemoInfo.CfgDat = ConfigDat;

	DemoBufPtr         = 0;
	DemoplaySaveEnable = TRUE;
}


bool DemoplayLoadSetup()
{
	DemoBufPtr = 0;
	DemoplayLoadEnable = true;

	// コンフィグの初期化 //
	// 現在のコンフィグを保持する //
	ConfigTemp = ConfigDat;

	// そのときのコンフィグを転送 //
	ConfigDat.BombStock.v   = DemoInfo.CfgDat.BombStock;
	ConfigDat.GameLevel.v   = DemoInfo.CfgDat.GameLevel;
	ConfigDat.PlayerStock.v = DemoInfo.CfgDat.PlayerStock;
	ConfigDat.InputFlags.v  = DemoInfo.CfgDat.InputFlags;

	// 本体の性能記述 //
	Viv.exp    = DemoInfo.Exp;
	Viv.weapon = DemoInfo.Weapon;
	Viv.left   = ConfigDat.PlayerStock.v;
	Viv.bomb   = ConfigDat.BombStock.v;

	// 乱数の初期化 //
	// 最後に乱数もそろえる //
	rnd_seed_set(DemoInfo.RndSeed);

	return true;
}


bool DemoplayRecord(INPUT_BITS key)
{
	if(!DemoplaySaveEnable) {
		return false;
	}

	DemoBuffer[DemoBufPtr++] = key;

	// バッファが最後に来たか、ＥＳＣが押された場合 //
	if(DemoBufPtr==DEMOBUF_MAX || (key&KEY_ESC)){
		DemoBufPtr--;
		return true;
	}
	return false;
}


void DemoplaySaveDemo(void)
{
	if(!DemoplaySaveEnable) return;

	DemoBuffer[DemoBufPtr] = KEY_ESC;
	DemoInfo.DataSize = DemoBufPtr+1;

	PATH_CODEUNIT fn[] = _PATH("STG_Demo.DAT");
	fn[3] = ('0' + GameStage);

	const std::array<BYTE_BUFFER_BORROWED, 2> bufs = {
		DemoInfo,
		std::span<INPUT_BITS>{ DemoBuffer, (DemoBufPtr + 1) },
	};
	FileWrite(fn, std::span<const BYTE_BUFFER_BORROWED>{ bufs });

	DemoplaySaveEnable = FALSE;
}


bool DemoplayLoadDemo(int stage)
{
	// Not too pretty… //
	BYTE_BUFFER_OWNED DATEnemyLoad(fil_no_t filno);

	// 展開 //
	const auto temp = DATEnemyLoad(stage-1+18);
	auto temp_cursor = temp.cursor();
	{
		const auto maybe_info = temp_cursor.next<DEMOPLAY_INFO>();
		if(!maybe_info) {
			return false;
		}
		DemoInfo = maybe_info.value()[0];
	}
	{
		const auto maybe_inputs = temp_cursor.next<uint16_t>(DemoInfo.DataSize);
		if(!maybe_inputs) {
			return false;
		}
		const auto inputs = maybe_inputs.value();
		memcpy(DemoBuffer, inputs.data(), inputs.size_bytes());
	}
	return DemoplayLoadSetup();
}


INPUT_BITS DemoplayMove(void)
{
	DWORD ptr;

	if(!DemoplayLoadEnable) return KEY_ESC;

	ptr = DemoBufPtr;
	if(ptr>=DemoInfo.DataSize){
		ptr = DemoBufPtr;
		DemoplayLoadEnable = FALSE;
	}
	else DemoBufPtr++;

	return DemoBuffer[ptr];
}


void DemoplayCleanup(void)
{
	ConfigDat          = ConfigTemp;
	DemoplayLoadEnable = FALSE;
}


void DemoplaySaveReplay(void)
{
	if(!DemoplaySaveEnable) return;

	// すぐさま、無効化する //
	DemoplaySaveEnable = FALSE;

	DemoBuffer[DemoBufPtr] = KEY_ESC;
	DemoInfo.DataSize = DemoBufPtr+1;

	const auto demo_size = (sizeof(INPUT_BITS) * DemoInfo.DataSize);
	PACKFILE_WRITE out = { {
		DemoInfo,
		std::span<INPUT_BITS>{ DemoBuffer, demo_size },
	} };

	const auto fn = ReplayFN(GameStage);
	out.Write(fn.c_str());
}


bool DemoplayLoadReplay(int stage)
{
	BYTE_BUFFER_OWNED	temp;

	const auto fn = ReplayFN(GameStage);
	const auto in = FilStartR(fn.c_str());

	// ヘッダの格納先は０番である //
	temp = in.MemExpand( 0);
	if(NULL == temp) return FALSE;
	memcpy(&DemoInfo, temp.get(), sizeof(DEMOPLAY_INFO));

	// データの格納先は１番ですね //
	temp = in.MemExpand( 1);
	if(NULL == temp){
		return FALSE;
	}
	memcpy(DemoBuffer, temp.get(), sizeof(WORD)*DemoInfo.DataSize);
	return DemoplayLoadSetup();
}
