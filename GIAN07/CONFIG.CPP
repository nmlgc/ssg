/*
 *   Config data
 *
 */

#pragma once

#include "CONFIG.H"
#include "LEVEL.H"

///// [グローバル変数] /////
CONFIG_DATA ConfigDat;
#ifdef PBG_DEBUG
	DEBUG_DATA DebugDat;
#endif

///// [ 関数(非公開) ] /////
static void ConfigInit(void)
{
	// 難易度関連 //
	ConfigDat.BombStock   = 2;
	ConfigDat.GameLevel   = GAME_NORMAL;
	ConfigDat.PlayerStock = 2;

	// グラフィック関連 //
	ConfigDat.DeviceID   = 0;
	ConfigDat.BitDepth   = DxEnum[ConfigDat.DeviceID].BitDepthBest();
	ConfigDat.DrawMode   = 1;		// 60Fps Mode
	ConfigDat.GraphFlags = 0;

	// サウンド・ＭＩＤＩ関連 //
	ConfigDat.SoundFlags = SNDF_WAVE_ENABLE|SNDF_MIDI_ENABLE;

	// 入力関連 //
	ConfigDat.InputFlags = INPF_Z_MSKIP_ENABLE;

	// デバッグ関連 //
	ConfigDat.DebugFlags = 0;

	// パッド関連 //
	ConfigDat.PadTama   = 1;
	ConfigDat.PadBomb   = 2;
	ConfigDat.PadShift  = 0;
	ConfigDat.PadCancel = 0;

	ConfigDat.ExtraStgFlags = 0;

	// その他 //
	ConfigDat.StageSelect = 0;
}

#ifdef PBG_DEBUG
static void DebugInit(void)
{
	FILE	*fp;

	fp = fopen("秋霜DBG.DAT","rb");
	if(fp==NULL){
		DebugDat.Hit        = TRUE;
		DebugDat.MsgDisplay = TRUE;
		DebugDat.DemoSave   = FALSE;
		DebugDat.StgSelect  = 1;
	}
	else{
		fread(&DebugDat,sizeof(DEBUG_DATA),1,fp);
		fclose(fp);
	}
}
#endif

// コンフィグの内容を初期化する //
extern void ConfigLoad(void)
{
	FILE		*fp;
	DWORD		tmp;

#ifdef PBG_DEBUG
	DebugInit();
#endif

	while(1){
		fp = fopen("秋霜CFG.DAT","rb");
		if(fp==NULL) break;

		fread(&ConfigDat,sizeof(CONFIG_DATA),1,fp);
		fclose(fp);

		tmp = 0;
		tmp += (ConfigDat.BitDepth.value() << 1);
		tmp += (ConfigDat.BombStock << 3);
		tmp += (ConfigDat.DeviceID << 2);
		tmp += (ConfigDat.DrawMode << 6);
		tmp += (ConfigDat.ExtraStgFlags << 5);
		tmp += (ConfigDat.GameLevel << 2);
		tmp += (ConfigDat.GraphFlags << 1);
		tmp += (ConfigDat.InputFlags << 3);
		tmp += (ConfigDat.PadBomb << 6);
		tmp += (ConfigDat.PadCancel << 3);
		tmp += (ConfigDat.PadShift << 1);
		tmp += (ConfigDat.PadTama << 4);
		tmp += (ConfigDat.PlayerStock << 5);
		tmp += (ConfigDat.SoundFlags << 2);
		if(tmp != ConfigDat.CheckSum) break;

		// 列挙したデバイス数と比較して異常ならここで修正する //
		// ここに来た場合は、ファイルが壊れているかビデオカードが抜かれたかの //
		// どちらかなので、強制的に break しない。                            //
		if(ConfigDat.DeviceID>=DxEnumNow) ConfigDat.DeviceID = 0;

		if(ConfigDat.PlayerStock>6) break;
		if(ConfigDat.PlayerStock<0) break;
		if(ConfigDat.BombStock>3) break;

		if(DxEnum[ConfigDat.DeviceID].BitDepthSupported(ConfigDat.BitDepth) == false) break;

		if(ConfigDat.DrawMode>3) break;
		if(ConfigDat.GameLevel>3) break;
		if(ConfigDat.GraphFlags&GRPF_MASK) break;

		if(ConfigDat.SoundFlags&SNDF_MASK) break;

		if(ConfigDat.InputFlags&INPF_MASK) break;

		if(ConfigDat.DebugFlags&DBGF_MASK) break;

		if(ConfigDat.PadTama   > 32) break;
		if(ConfigDat.PadBomb   > 32) break;
		if(ConfigDat.PadShift  > 32) break;
		if(ConfigDat.PadCancel > 32) break;

		// これだけは、特別扱い //
		if(ConfigDat.StageSelect > 6) ConfigDat.StageSelect = 0;

		return;
	}

	ConfigInit();
}

// コンフィグの内容を保存する //
extern void ConfigSave(void)
{
	FILE		*fp;
	BYTE		*p;
	DWORD		sum;

	fp = fopen("秋霜CFG.DAT","wb");
	if(fp==NULL) return;

	ConfigDat.CheckSum = 0;
	ConfigDat.CheckSum += (ConfigDat.BitDepth.value() << 1);
	ConfigDat.CheckSum += (ConfigDat.BombStock << 3);
	ConfigDat.CheckSum += (ConfigDat.DeviceID << 2);
	ConfigDat.CheckSum += (ConfigDat.DrawMode << 6);
	ConfigDat.CheckSum += (ConfigDat.ExtraStgFlags << 5);
	ConfigDat.CheckSum += (ConfigDat.GameLevel << 2);
	ConfigDat.CheckSum += (ConfigDat.GraphFlags << 1);
	ConfigDat.CheckSum += (ConfigDat.InputFlags << 3);
	ConfigDat.CheckSum += (ConfigDat.PadBomb << 6);
	ConfigDat.CheckSum += (ConfigDat.PadCancel << 3);
	ConfigDat.CheckSum += (ConfigDat.PadShift << 1);
	ConfigDat.CheckSum += (ConfigDat.PadTama << 4);
	ConfigDat.CheckSum += (ConfigDat.PlayerStock << 5);
	ConfigDat.CheckSum += (ConfigDat.SoundFlags << 2);

	fwrite(&ConfigDat,sizeof(CONFIG_DATA),1,fp);
	fclose(fp);

#ifdef PBG_DEBUG
	fp = fopen("秋霜DBG.DAT","wb");
	if(fp==NULL) return;
	fwrite(&DebugDat,sizeof(DEBUG_DATA),1,fp);
	fclose(fp);
#endif
}
