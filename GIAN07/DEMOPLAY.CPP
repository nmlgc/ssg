/*                                                                           */
/*   DemoPlay.cpp   デモプレイ処理                                           */
/*                                                                           */
/*                                                                           */

#include "platform/file.h"
#include "CONFIG.H"
#include "LZ_UTY.H"
#include "GIAN.H"
#include "DEMOPLAY.H"
#include "game/input.h"


bool	DemoplaySaveEnable = false;	// デモプレイのセーブが動作しているか
bool	DemoplayLoadEnable = false;	// デモプレイのロードが動作しているか
DEMOPLAY_INFO	DemoInfo;						// デモプレイ情報
INPUT_BITS	DemoBuffer[DEMOBUF_MAX];	// デモプレイ用バッファ
size_t	DemoBufPtr;	// デモプレイ用バッファPtr(Ｃのそれではない)
CONFIG_DATA		ConfigTemp;						// コンフィグのデータ一時保存用

//DWORD RndBuf[RNDBUF_MAX];

void DEMOPLAY_CONFIG_DATA::operator=(const CONFIG_DATA& config)
{
	GameLevel = config.GameLevel.v;
	PlayerStock = config.PlayerStock.v;
	BombStock = config.BombStock.v;
	InputFlags = config.InputFlags.v;
}



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
	DemoInfo.RndSeed = (
		(Cast::up<uint32_t>(rnd()) + 1u) * (Cast::up<uint32_t>(rnd()) + 1u)
	);
	rnd_seed_set(DemoInfo.RndSeed);

	DemoInfo.Exp    = Viv.exp;
	DemoInfo.Weapon = Viv.weapon;
	DemoInfo.CfgDat = ConfigDat;

	DemoBufPtr         = 0;
	DemoplaySaveEnable = true;
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
	ConfigDat.PlayerStock.v = DemoInfo.CfgDat.PlayerStock;
	ConfigDat.InputFlags.v  = DemoInfo.CfgDat.InputFlags;
	GameLevel = DemoInfo.CfgDat.GameLevel;

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
		std::span(&DemoInfo, 1), std::span(DemoBuffer, (DemoBufPtr + 1)),
	};
	FileWrite(fn, std::span<const BYTE_BUFFER_BORROWED>{ bufs });

	DemoplaySaveEnable = false;
}


bool DemoplayLoadDemo(int stage)
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
	if(!DemoplayLoadEnable) return KEY_ESC;

	const auto ptr = DemoBufPtr;
	if(ptr>=DemoInfo.DataSize){
		DemoplayLoadEnable = false;
	}
	else DemoBufPtr++;

	return DemoBuffer[ptr];
}


void DemoplayCleanup(void)
{
	ConfigDat          = ConfigTemp;
	DemoplayLoadEnable = false;
}


void DemoplaySaveReplay(void)
{
	if(!DemoplaySaveEnable) return;

	// すぐさま、無効化する //
	DemoplaySaveEnable = false;

	DemoBuffer[DemoBufPtr] = KEY_ESC;
	DemoInfo.DataSize = DemoBufPtr+1;

	const auto demo_size = (sizeof(INPUT_BITS) * DemoInfo.DataSize);
	PACKFILE_WRITE out = { {
		std::span(&DemoInfo, 1), std::span(DemoBuffer, demo_size),
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
	if(NULL == temp) {
		return false;
	}
	memcpy(&DemoInfo, temp.get(), sizeof(DEMOPLAY_INFO));

	// データの格納先は１番ですね //
	temp = in.MemExpand( 1);
	if(NULL == temp){
		return false;
	}
	memcpy(DemoBuffer, temp.get(), (sizeof(INPUT_BITS) * DemoInfo.DataSize));
	return DemoplayLoadSetup();
}
