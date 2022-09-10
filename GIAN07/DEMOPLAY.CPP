/*                                                                           */
/*   DemoPlay.cpp   デモプレイ処理                                           */
/*                                                                           */
/*                                                                           */

#include "LZ_UTY.H"
#include "DEMOPLAY.H"


BOOL			DemoplaySaveEnable = FALSE;		// デモプレイのセーブが動作しているか
BOOL			DemoplayLoadEnable = FALSE;		// デモプレイのロードが動作しているか
DEMOPLAY_INFO	DemoInfo;						// デモプレイ情報
WORD			DemoBuffer[DEMOBUF_MAX];		// デモプレイ用バッファ
DWORD			DemoBufPtr;						// デモプレイ用バッファPtr(Ｃのそれではない)
CONFIG_DATA		ConfigTemp;						// コンフィグのデータ一時保存用

//DWORD RndBuf[RNDBUF_MAX];



// デモプレイデータの準備 //
FVOID DemoplayInit(void)
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


// デモプレイデータを保存する //
FVOID DemoplayRecord(WORD key)
{
	if(!DemoplaySaveEnable) return;

	DemoBuffer[DemoBufPtr++] = key;

	// バッファが最後に来たか、ＥＳＣが押された場合 //
	if(DemoBufPtr==DEMOBUF_MAX || (key&KEY_ESC)){
		DemoBufPtr--;
		DemoplaySave();
		DemoplaySaveEnable = FALSE;
	}
}


// デモプレイデータをファイルに書き込む //
FVOID DemoplaySave(void)
{
	char		buf[100];
	FILE		*fp;

	if(!DemoplaySaveEnable) return;

	DemoBuffer[DemoBufPtr] = KEY_ESC;
	DemoInfo.DataSize = DemoBufPtr+1;

	sprintf(buf,"STG%dDemo.DAT",GameStage);
	fp = fopen(buf,"wb");
	if(fp==NULL) return;

	fwrite(&DemoInfo,sizeof(DEMOPLAY_INFO),1,fp);
	fwrite(DemoBuffer,sizeof(WORD)*(DemoBufPtr+1),1,fp);
	fclose(fp);

	DemoplaySaveEnable = FALSE;
}


// デモプレイデータをロードする //
FBOOL DemoplayLoad(int stage)
{
	// ファイルを開く //
	const auto in = FilStartR("ENEMY.DAT");

	// 展開 //
	const auto temp = in.MemExpand(stage-1+18);
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
		const auto& inputs = maybe_inputs.value();
		memcpy(DemoBuffer, inputs.data(), inputs.size_bytes());
	}

	DemoBufPtr = 0;
	DemoplayLoadEnable = TRUE;

	// コンフィグの初期化 //
	ConfigTemp = ConfigDat;

	ConfigDat.BombStock   = DemoInfo.CfgDat.BombStock;
	ConfigDat.GameLevel   = DemoInfo.CfgDat.GameLevel;
	ConfigDat.PlayerStock = DemoInfo.CfgDat.PlayerStock;
	ConfigDat.InputFlags  = DemoInfo.CfgDat.InputFlags;

	Viv.exp    = DemoInfo.Exp;
	Viv.weapon = DemoInfo.Weapon;
	Viv.left   = ConfigDat.PlayerStock;
	Viv.bomb   = ConfigDat.BombStock;

	// 乱数の初期化 //
	rnd_seed_set(DemoInfo.RndSeed);

	return TRUE;
}


// Key_Data を返す //
FWORD DemoplayMove(void)
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


// デモプレイロードの事後処理 //
FVOID DemoplayLoadCleanup(void)
{
	ConfigDat          = ConfigTemp;
	DemoplayLoadEnable = FALSE;
}


// リプレイデータの準備 //
FVOID ReplayInit(void)
{
	// これは同じ //
	DemoplayInit();
}


// リプレイデータを保存する //
FVOID ReplayRecord(WORD key)
{
	if(!DemoplaySaveEnable) return;

	DemoBuffer[DemoBufPtr++] = key;

	if(DemoBufPtr == DEMOBUF_MAX || (key&KEY_ESC)){
		DemoBufPtr--;
		ReplaySave();					// 強制吐き出し
		DemoplaySaveEnable = FALSE;
	}
}


// リプレイデータをファイルに書き込む //
FVOID ReplaySave(void)
{
	char			buf[100];

	if(!DemoplaySaveEnable) return;

	// すぐさま、無効化する //
	DemoplaySaveEnable = FALSE;

	DemoBuffer[DemoBufPtr] = KEY_ESC;
	DemoInfo.DataSize = DemoBufPtr+1;

	// ファイル名を確定 //
	if(GameStage == GRAPH_ID_EXSTAGE) strcpy(buf, "秋霜りぷEx.DAT");
	else                              sprintf(buf, "秋霜りぷ%d.DAT", GameStage);

	const auto demo_size = (sizeof(WORD) * DemoInfo.DataSize);
	PACKFILE_WRITE out = { {
		{ reinterpret_cast<uint8_t *>(&DemoInfo), sizeof(DEMOPLAY_INFO) },
		{ reinterpret_cast<uint8_t *>(DemoBuffer), demo_size }
	} };
	out.Write(buf);
}


// リプレイデータをロードする //
FBOOL ReplayLoad(int stage)
{
	BYTE_BUFFER_OWNED	temp;
	char			buf[100];

	// ファイル名を確定 //
	if(GameStage == GRAPH_ID_EXSTAGE) strcpy(buf, "秋霜りぷEx.DAT");
	else                              sprintf(buf, "秋霜りぷ%d.DAT", stage);

	const auto in = FilStartR(buf);

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


	DemoBufPtr = 0;
	DemoplayLoadEnable = TRUE;

	// 現在のコンフィグを保持する //
	ConfigTemp = ConfigDat;

	// そのときのコンフィグを転送 //
	ConfigDat.BombStock   = DemoInfo.CfgDat.BombStock;
	ConfigDat.GameLevel   = DemoInfo.CfgDat.GameLevel;
	ConfigDat.PlayerStock = DemoInfo.CfgDat.PlayerStock;
	ConfigDat.InputFlags  = DemoInfo.CfgDat.InputFlags;

	// 本体の性能記述 //
	Viv.exp    = DemoInfo.Exp;
	Viv.weapon = DemoInfo.Weapon;
	Viv.left   = ConfigDat.PlayerStock;
	Viv.bomb   = ConfigDat.BombStock;

	// 最後に乱数もそろえる //
	rnd_seed_set(DemoInfo.RndSeed);

	return TRUE;
}


// Key_Data を返す //
FWORD ReplayMove(void)
{
	DWORD	ptr;

	if(!DemoplayLoadEnable) return KEY_ESC;

	ptr = DemoBufPtr;

	if(ptr >= DemoInfo.DataSize){
		ptr = DemoBufPtr;
		DemoplayLoadEnable = FALSE;
	}
	else
		DemoBufPtr++;

	return DemoBuffer[ptr];
}


// リプレイデータロードの事後処理 //
FVOID ReplayLoadCleanup(void)
{
	ConfigDat = ConfigTemp;
	DemoplayLoadEnable = FALSE;
}
