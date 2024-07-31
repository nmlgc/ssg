/*                                                                           */
/*   GameMain.cpp   ウィンドウシステム切り替えなどの処理                     */
/*                                                                           */
/*                                                                           */

#include "GAMEMAIN.H"
#include "CONFIG.H"
#include "DEMOPLAY.H"
#include "GIAN.H"
#include "LENS.H"
#include "LEVEL.H"
#include "MUSIC.H"
#include "SCORE.H"
#include "game/bgm.h"
#include "game/input.h"
#include "game/midi.h"
#include "game/snd.h"
#include <mmsystem.h>


DWORD		DrawCount = 0;
DWORD		MidiPlayID = 0;
DWORD		DemoTimer  = 0;
BYTE		WeaponKeyWait = 0;
int	GameOverTimer = 0;

NR_NAME_DATA	CurrentName;	// ネームレジスト準備用データ
BYTE			CurrentRank;	// ネームレジスト用順位データ
BYTE			CurrentDif;		// 現在の難易度(スコアネーム表示用)

MAID		VivTemp;

bool IsDemoplay = false;


void TitleProc(bool& quit);
void WeaponSelectProc(bool&);	// 装備選択
void GameProc(bool&);
void GameOverProc0(bool&);	// ゲームオーバー出現用
void GameOverProc(bool&);	// ゲームオーバー
void PauseProc(bool&);
void MusicRoomProc(bool&);	// 音楽室
void DemoProc(bool&);	// デモプレイ
void NameRegistProc(bool&);	// お名前入力

void ReplayProc(bool&);


bool SProjectInit(void);	// 西方Ｐｒｏｊｅｃｔ初期化部
void SProjectProc(bool&);	// 西方Ｐｒｏｊｅｃｔ表示動作部

void ScoreNameProc(bool&);

void GameSTD_Init(void);	// ゲームを立ち上げる際に必ず行う初期化関数群
bool DemoInit(void);	// デモプレイの初期化を行う

void GameDraw(void);
void GameMove(void);
bool IsDraw(void);

void ScoreDraw(void);	// スコアの描画

void(*GameMain)(bool& quit) = TitleProc;

uint8_t CurrentLevel()
{
	return ((GameStage == GRAPH_ID_EXSTAGE) ? GAME_EXTRA : GameLevel);
}

bool InputLocked;


// スコアネーム表示の準備を行う //
extern bool ScoreNameInit(void)
{
	CurrentDif = CurrentLevel();

	CurrentRank = (BYTE)SetScoreString(NULL, CurrentDif);
	if(CurrentRank == 0) return GameExit();

	MWinForceClose();
	GrpCls();
	GrpFlip();

	if(!LoadGraph(GRAPH_ID_NAMEREGIST)){
		DebugOut(u8"GRAPH.DAT が破壊されています");
		return FALSE;
	}

	GrpSetClip(0,0,640,480);

	InputLocked = Key_Data;
	GameMain = ScoreNameProc;

	return TRUE;
}

// スコアネームの表示 //
void ScoreNameProc(bool&)
{
	static const char *ExString[5] = {"Easy","Normal","Hard","Lunatic","Extra"};

	switch(Key_Data){
		case(KEY_RETURN):case(KEY_TAMA):case(KEY_BOMB):case(KEY_ESC):
			if(InputLocked) {
				break;
			}
			Snd_SEPlay(SOUND_ID_CANCEL);
			GameExit(FALSE);
		return;

		case(KEY_UP):case(KEY_LEFT):
			if(ScoreString[4].bMoveEnable) break;
			Snd_SEPlay(SOUND_ID_SELECT);
			CurrentDif  = (CurrentDif+4)%5;
			CurrentRank = (BYTE)SetScoreString(NULL, CurrentDif);
		break;

		case(KEY_DOWN):		case(KEY_RIGHT):
			if(ScoreString[4].bMoveEnable) break;
			Snd_SEPlay(SOUND_ID_SELECT);
			CurrentDif  = (CurrentDif+1)%5;
			CurrentRank = (BYTE)SetScoreString(NULL, CurrentDif);
		break;

		case 0:
			InputLocked = false;
			break;
	}

	GrpCls();
	ScoreDraw();
	GrpPut16(320,450,ExString[CurrentDif]);
	GrpFlip();
}

// スコアの描画 //
void ScoreDraw(void)
{
	int		i,gx,gy,v;
	PIXEL_LTRB	src;

	for(i=0;i<5;i++){
		v = (ScoreString[i].x - ((50  + i*24) << 6))/12;
		if(v > 64*2){
			//v = max(v,20*64);
			ScoreString[i].x -= v;
		}
		else
			ScoreString[i].bMoveEnable = FALSE;

		src = PIXEL_LTWH{ 0, (64 + (32 * i)), 400, 32 };
		GrpBlt(
			&src, (ScoreString[i].x >> 6), (ScoreString[i].y >> 6), GrNameReg
		);
		// GrpBlt(&src, (50 + (i * 24)), (100 + (i * 48), GrNameReg);

		gx = (ScoreString[i].x>>6) + 88;
		gy = (ScoreString[i].y>>6) + 4;
		GrpPut16c2(gx, gy, ScoreString[i].Name);

		gx = (ScoreString[i].x>>6) + 232-16;
		gy = (ScoreString[i].y>>6) + 4;
		GrpPut16c2(gx, gy, ScoreString[i].Score);

		gx = (ScoreString[i].x>>6) + 120;
		gy = (ScoreString[i].y>>6) + 25;
		GrpPutScore(gx, gy, ScoreString[i].Evade);

		// いや、時間が無いのは分かるんだけどさぁ... //
		gx = (ScoreString[i].x>>6) + 224;
		gy = (ScoreString[i].y>>6) + 25;
		if(ScoreString[i].Stage[0] == '7'){
			src = { 288, 88, (288 + 16), (88 + 8) };
			GrpBlt(&src, gx, gy-1, GrTama);
		}
		else
			GrpPutScore(gx, gy, ScoreString[i].Stage);

		gx = (ScoreString[i].x>>6) + 224 + 80;
		gy = (ScoreString[i].y>>6) + 25;
		src = PIXEL_LTWH{ 0, (400 + (ScoreString[i].Weapon * 8)), 48, 8 };
		GrpBlt(&src, gx, gy-1, GrNameReg);
	}
}


#define NR_EXCHAR_BACK		0
#define NR_EXCHAR_END		-1
#define NR_EXCHAR_ERROR		-2

// 座標から選択文字を取得する //
char GetAddr2Char(int x, int y)
{
	// 大文字 //
	if(y == 0){
		return (char)('A' + (x % 26));
	}
	// 小文字 //
	else if (y == 1){
		return (char)('a' + (x % 26));
	}
	// その他記号など //
	else{
		switch(x){
		case 0:		return '0';
		case 1:		return '1';
		case 2:		return '2';
		case 3:		return '3';
		case 4:		return '4';
		case 5:		return '5';
		case 6:		return '6';
		case 7:		return '7';
		case 8:		return '8';
		case 9:		return '9';
		case 10:	return '!';
		case 11:	return '?';
		case 12:	return '#';
		case 13:	return '\\';
		case 14:	return '<';
		case 15:	return '>';
		case 16:	return '=';
		case 17:	return ',';
		case 18:	return '+';
		case 19:	return '-';
		case 20:	return ' ';		// SPACE
		//case 21:
		case 22:	return NR_EXCHAR_BACK;
		//case 23:
		case 24:	return NR_EXCHAR_END;
		//case 25:
		default:	return NR_EXCHAR_ERROR;
		}
	}
}

// お名前入力 //
void NameRegistProc(bool&)
{
	// <- DemoInit() を修正するのだぞ
	PIXEL_LTRB	src = {0, 0, 400, 64};
	int		gx,gy,len;
	static int	x,y;
	static int8_t key_time;
	static BYTE count;
	static BYTE time;
	//char	buf[100],
	char	c;

	constexpr int8_t END_WAIT = -1;

	if(key_time==0){
		key_time = 8;//16;

		switch(Key_Data){
			case(KEY_UP):
				y = (y+2)%3;
				Snd_SEPlay(SOUND_ID_SELECT);
			break;

			case(KEY_DOWN):
				y = (y+1)%3;
				Snd_SEPlay(SOUND_ID_SELECT);
			break;

			case(KEY_LEFT):
				if(y==2 && x>20)	x = (x-2)%26;
				else				x = (x+25)%26;
				Snd_SEPlay(SOUND_ID_SELECT);
			break;

			case(KEY_RIGHT):
				if(y==2 && x>=20)	x = (x+2)%26;
				else				x = (x+1)%26;
				Snd_SEPlay(SOUND_ID_SELECT);
			break;

			case(KEY_BOMB):
				Snd_SEPlay(SOUND_ID_CANCEL);
			goto BACK_NR_PROC;

			case(KEY_TAMA):case(KEY_RETURN):
				if(InputLocked) {
					break;
				}
				Snd_SEPlay(SOUND_ID_SELECT);

				// 最後の文字まで来ていた場合 //
				if(strlen(ScoreString[CurrentRank-1].Name)==NR_NAME_LEN-1){
					switch(c = GetAddr2Char(x,y)){
						case(NR_EXCHAR_END): case(NR_EXCHAR_ERROR):
						goto EXIT_NR_PROC;

						case(NR_EXCHAR_BACK):
						goto BACK_NR_PROC;

						default:
							x = 24;
							y = 2;
						break;
					}

					break;
				}

				// それ以外の場合 //
				switch(c = GetAddr2Char(x,y)){
					case(NR_EXCHAR_END): case(NR_EXCHAR_ERROR):
					goto EXIT_NR_PROC;

					case(NR_EXCHAR_BACK):
					goto BACK_NR_PROC;

					default:
						len = strlen(ScoreString[CurrentRank-1].Name);
						ScoreString[CurrentRank-1].Name[len  ] = c;
						ScoreString[CurrentRank-1].Name[len+1] = '\0';
					break;
				}
			break;

			// １文字前に戻る //
			BACK_NR_PROC:
				len = strlen(ScoreString[CurrentRank-1].Name);
				if(len) ScoreString[CurrentRank-1].Name[len-1] = '\0';
			break;

			// ネームレジスト終了処理 //
			EXIT_NR_PROC:
				if(strlen(ScoreString[CurrentRank-1].Name)==0)
					strcpy(ScoreString[CurrentRank-1].Name, "Vivit!");

				ScoreString[CurrentRank-1].Name[NR_NAME_LEN-1] = '\0';

				strcpy(CurrentName.Name, ScoreString[CurrentRank-1].Name);
				SaveScoreData(&CurrentName, CurrentLevel());

				key_time = END_WAIT;
				break;

			case 0:
				InputLocked = false;
				break;
		}

		if(x>20 && y==2)	x &= (~1);
	} else if(key_time != END_WAIT) {
		key_time--;
	}

	if(key_time == -1) {
		if(Key_Data == 0) {
			x = y = key_time = 0;
			GameExit();
			return;
		}
	} else {
		if(Key_Data == 0) {
			key_time = 0;
		}
		count = ((count + 1) % 24);
		time++;
	}

	GrpCls();

	GrpLock();

	GrpSetColor(2,0,0);
	gx = ScoreString[CurrentRank-1].x >> 6;
	gy = ScoreString[CurrentRank-1].y >> 6;
	GrpBox(gx,gy,gx+400,gy+32);

	if(time%64 > 32){
		GrpSetColor(4,0,0);
		len = min(strlen(ScoreString[CurrentRank-1].Name), NR_NAME_LEN-2);
		gx += (len*16 + 88);
		gy += (4);
		GrpBox(gx,gy,gx+14,gy+16);
	}

	GrpUnlock();

	GrpBlt(&src, 120, 0, GrNameReg);

	ScoreDraw();

	// 名前入力用文字列群 //
	src = { 0, 432, 416, 480 };
	GrpBlt(&src, 112, 420, GrNameReg);

	// カーソル //
	if((x >= 20) && (y == 2)) {
		src = PIXEL_LTWH{ 432, (432 + ((count >> 3) << 4)), 32, 16 };
	} else {
		src = PIXEL_LTWH{ 416, (432 + ((count >> 3) << 4)), 16, 16 };
	}
	GrpBlt(&src, (112 + (x << 4)), (420 + (y << 4)), GrNameReg);

	//sprintf(buf,"(%2d,%2d)", x, y);
	//GrpPut16(0,0,buf);

/*
	GrpPut16(400,100,temps);
	for(i=0; i<5; i++){
		GrpPut16(100, 100+i*32, ScoreString[i].Score);
		if(CurrentRank == i+1) GrpPut16(85, 100+i*32, "!!");
	}
*/
	GrpFlip();
}

// お名前入力の初期化 //
bool NameRegistInit(bool bNeedChgMusic)
{
	for(auto& it : CurrentName.Name) {
		it = '\0';
	}
	CurrentName.Score  = Viv.score;
	CurrentName.Evade  = Viv.evade_sum;
	CurrentName.Weapon = Viv.weapon;
	if(GameStage == GRAPH_ID_EXSTAGE) CurrentName.Stage = 1;
	else                              CurrentName.Stage = GameStage;

	// デバッグ用... //
	Snd_SEStop(8);	// ワーニング音を止める
	Snd_SEStopAll();

	// ハイスコアで無いならばタイトルに移行する //
	CurrentRank = (BYTE)SetScoreString(&CurrentName, CurrentLevel());
	if(CurrentRank == 0) return GameExit();

	MWinForceClose();
	GrpCls();
	GrpFlip();

	if(!LoadGraph(GRAPH_ID_NAMEREGIST)){
		DebugOut(u8"GRAPH.DAT が破壊されています");
		return FALSE;
	}

	GrpSetClip(0,0,640,480);

	InputLocked = Key_Data;
	GameMain = NameRegistProc;

	if(bNeedChgMusic){
		BGM_Switch(19);
	}

	return TRUE;
}

// ゲームを立ち上げる際に必ず行う初期化関数群 //
void GameSTD_Init(void)
{
	MWinForceClose();
	//GrpCls();
	//GrpFlip();

	BossDataInit();

	//MaidSet();
	MaidTamaIndSet();
	enemyind_set();
	tamaind_set(400+200);		// 小型弾に４００
	laserind_set();
	LLaserSetup();
	HLaserInit();
	SEffectInit();
	CEffectInit();
	ObjectLockOnInit();
	ItemIndSet();
	fragment_setup();
	ScreenEffectInit();
	ScreenEffectSet(SCNEFC_CFADEIN);

	ExBombEfcInit();

	InitWarning();
	WarningEffectInit();
	//WarningEffectSet();

	BGM_SetTempo(0);

	//DrawCount = 0;
}


bool WeaponSelectInit(bool ExStg)
{
	GrpCls();
	GrpFlip();

	GameLevel = (ExStg ? GAME_HARD : ConfigDat.GameLevel.v);

	GameSTD_Init();
	PlayRankReset();

	MaidSet();

	GrpSetClip(0,0,640,480);

	WeaponKeyWait = 1;
	Viv.weapon    = 0;
	GameMain = WeaponSelectProc;
	if(ExStg) {
		GameStage = GRAPH_ID_EXSTAGE;
	}

	VivTemp = Viv;

	return TRUE;
}

bool GameInit(void(*NextProc)(bool& quit))
{
	TextObj.Clear();
	if(NextProc != DemoProc) {
		BGM_FadeOut(240);
		MTitleInit();
	}
	if(NextProc == GameProc) {
		// ウィンドウの表示位置を設定する //
		// Replays don't show dialog, so this is the only place where we need
		// to do this.
		const auto flags = MSG_WINDOW_FLAGS::WITH_FACE;
		if(ConfigDat.GraphFlags.v & GRPF_WINDOW_UPPER) {
			MWinInit({ 128,  16, (640 - 128),  96 }, flags);
		} else if(!(ConfigDat.GraphFlags.v & GRPF_MSG_DISABLE)) {
			MWinInit({ 128, 400, (640 - 128), 480 }, flags);
		}

		// These windows can also only show up outside of replays.
		InitExitWindow();
		InitContinueWindow();
	}
	GrpSetClip(X_MIN, Y_MIN, X_MAX, Y_MAX);
	GameMain = NextProc;
	return TRUE;
}

// 次のステージに移行する //
extern bool GameNextStage(void)
{
#ifdef PBG_DEBUG
	DemoplaySaveDemo();
#endif

	GameStage++;

	// エンディングに移行する //
	if(GameStage>=STAGE_MAX){
		GameStage = STAGE_MAX;	// 後で変更のこと
	}

	GameSTD_Init();
	MaidNextStage();

	if(!LoadGraph(GameStage)){
		DebugOut(u8"GRAPH.DAT が破壊されています");
		return FALSE;
	}
	if(!LoadStageData(GameStage)){
		DebugOut(u8"ENEMY.DAT が破壊されています");
		return FALSE;
	}

	return TRUE;
}


// リプレイ用の初期化を行う //
extern bool GameReplayInit(int Stage)
{
	MaidSet();

//	rnd_seed_set(timeGetTime());
	GameStage = Stage;

	if(!DemoplayLoadReplay(GameStage)) {
		// DebugOut(u8"デモプレイデータが存在せず");
		return FALSE;
	}

	PlayRankReset();

	GrpCls();
	GrpFlip();
	GameSTD_Init();

	if(!LoadGraph(GameStage)){
		DebugOut(u8"GRAPH.DAT が破壊されています");
		DemoplayCleanup();
		return FALSE;
	}
	if(!LoadStageData(GameStage)){
		DebugOut(u8"ENEMY.DAT が破壊されています");
		DemoplayCleanup();
		return FALSE;
	}

	if(GameStage == GRAPH_ID_EXSTAGE){
		Viv.credit = 0;
	}

	return GameInit(ReplayProc);
}

void ReplayProc(bool&)
{
	static BYTE ExTimer;
	PIXEL_LTRB	rc;

	ExTimer = (ExTimer+1)%128;

	if(Key_Data != KEY_ESC) {
		Key_Data = DemoplayMove();
	}

	// ＥＳＣが押されたら即、終了 //
	if(Key_Data & KEY_ESC){
		DemoplayCleanup();
		GameExit();
		return;
	}

	GameMove();

	if(GameMain != ReplayProc){
		DemoplayCleanup();	// 後始末
		GameExit();					// 強制終了させる(ゲームオーバー対策)
		return;
	}

	if(IsDraw()){
		GameDraw();

		rc = PIXEL_LTWH{ 312, 80, 32, 8 };
		GrpBlt(&rc,128,470,GrTama);
		if(ExTimer<64+32){
			GrpLock();
			GrpSetAlpha(128, ALPHA_NORM);
			GrpSetColor(0,0,0);
			GrpBoxA(128+45-3, 470+4-1, 128+45+72, 470+4+5);
			GrpUnlock();
			rc = PIXEL_LTWH{ 312, 88, 72, 8 };
			GrpBlt(&rc,128+45,470+4,GrTama);
		}
		GrpFlip();
	}
}


// デモプレイの初期化を行う //
bool DemoInit(void)
{
//	return NameRegistInit();

	GrpCls();
	GrpFlip();

	GameSTD_Init();

	MaidSet();

	rnd_seed_set(timeGetTime());
	GameStage = (rnd()%STAGE_MAX)+1;

//	GameStage = (rnd()%3)+1;		// この部分は体験版(コミケ)だけ //
//	GameStage = 1;					// こっちはＨＰ体験版用         //

	if(!DemoplayLoadDemo(GameStage)) {
		// DebugOut(u8"デモプレイデータが存在せず");
		return FALSE;
	}

	PlayRankReset();

	if(!LoadGraph(GameStage)){
		DebugOut(u8"GRAPH.DAT が破壊されています");
		return FALSE;
	}
	if(!LoadStageData(GameStage)){
		DebugOut(u8"ENEMY.DAT が破壊されています");
		return FALSE;
	}

	return GameInit(DemoProc);
}

// 西方Ｐｒｏｊｅｃｔ表示動作部 //
std::optional<LensInfo> Lens;

void SProjectProc(bool&)
{
	static WORD timer = 0;

	constexpr PIXEL_SIZE logo_size = { 320, 42 };
	constexpr WINDOW_LTRB logo = WINDOW_LTWH{
		(320 - (logo_size.w / 2)), (240 + 40), logo_size.w, logo_size.h
	};

	const auto fade = [logo](uint8_t black_alpha, uint8_t palette_tone) {
		if(DxObj.PixelFormat.IsChanneled()){
			GrpLock();
			GrpSetAlpha(black_alpha, ALPHA_NORM);
			GrpSetColor(0, 0, 0);
			GrpBoxA(logo.left, logo.top, logo.right, logo.bottom);
			GrpUnlock();
		} else {
			GrpSetPalette(SProjectPalette.Fade(palette_tone));
		}
	};

	PIXEL_LTRB rc = { 0, 0, logo_size.w, logo_size.h };
	BYTE	d;
	int		x, y;

	timer = timer + 1;

	if(timer >= 256){
		timer   = 0;

		Lens = std::nullopt;
		GameExit();
		return;
	}

	if(IsDraw()){
		GrpCls();//Ex(NULL, 255);

		GrpBlt(&rc, logo.left, logo.top, GrSProject);

		if(timer < 64){
			fade(((255 - timer) * 4), (timer * 4));
		}
		else if(timer > 192){
			fade((timer * 4), ((255 - timer) * 4));
		}
		else if(Lens) {
			d = timer - 64;
			x = 320 + sinl(d-64, 240);
			y = 295 + sinl(d*2, 20);
			Lens.value().Draw({ x, y });
		}

		GrpFlip();
	}
}


// 西方Ｐｒｏｊｅｃｔ表示の初期化 //
bool SProjectInit(void)
{
	if(!LoadGraph(GRAPH_ID_SPROJECT)){
		DebugOut(u8"GRAPH.DAT が破壊されています");
		return FALSE;
	}

	// レンズをすでに作成しているのなら、破棄する //
	Lens = GrpCreateLensBall(70, 36, GrLens);
	if(!Lens) {
		return false;
	}

	GameMain = SProjectProc;

	return TRUE;
}


// 音楽室の初期化をする //
extern bool MusicRoomInit(void)
{
	TextObj.Clear();
	MusicRoomText = MUSICROOM_TEXT::Register();

	MWinForceClose();
	GrpCls();
	GrpFlip();

	if(!LoadGraph(GRAPH_ID_MUSICROOM)){
		DebugOut(u8"GRAPH.DAT が破壊されています");
		return FALSE;
	}

	GrpSetClip(0,0,640,480);

	MidiPlayID = 0;

	BGM_SetTempo(0);

	// Still necessary because the note arrays aren't actually processed
	// outside of the Music Room.
	Mid_TableInit();
/*
	BGM_Stop();
*/
	if(!LoadMusicRoomComment(0)){
		DebugOut(u8"ENEMY.DAT がはかいされています");
		GameExit();
		return FALSE;
	}
/*
	if(!LoadMusic(0)){
		DebugOut(u8"MUSIC.DAT が破壊されています");
		GameExit();
		return FALSE;
	}
*/
//	BGM_Play();

	GameMain = MusicRoomProc;

	return TRUE;
}

// ゲームを再開する(ESC 抜けから) //
extern void GameRestart(void)
{
	GameMain = GameProc;
}

// ゲームから抜ける //
extern bool GameExit(bool bNeedChgMusic)
{
	TextObj.Clear();
	GrpCls();
	GrpFlip();

	if(!LoadGraph(GRAPH_ID_TITLE)){
		DebugOut(u8"GRAPH.DAT が破壊されています");
		return FALSE;
	}
	GrpSetClip(0,0,640,480);

	LLaserSetup();		// 音を止める
	Snd_SEStop(8);	// ワーニング音を止めるのだ

	const auto flags = MSG_WINDOW_FLAGS::CENTER;
	MWinForceClose();
	MWinInit({ (128 + 8), (400 + 16 + 20), (640 - 128 - 8), 480 }, flags);
	MWinOpen();
	//MWinFace(0);

	DemoTimer = 0;

	GameStage = 0;

	if(GameMain != DemoProc){
		if(bNeedChgMusic){
			BGM_Switch(0);
		}
	}

	// Must come after the BGM switch to correctly initialize the sound
	// configuration menu.
	InitMainWindow();
	MainWindow.Open({ 400, 250 }, 0);
	// MainWindow.Open({ 150, 200 }, 0);
	// MainWindow.Open({ 250, 150 }, 0);

	GameMain  = TitleProc;

	return TRUE;
}

// ゲームオーバーの前処理
extern void GameOverInit(void)
{
	StringEffect3();

	GameOverTimer = 120;

	GameMain = GameOverProc0;
}

// コンティニューを行う場合
extern void GameContinue(void)
{
	Viv.evade_sum = 0;
	Viv.left      = ConfigDat.PlayerStock.v;
	Viv.score     = (Viv.score%10 + 1);

	GameMain = GameProc;

	// ここに入らなかったらバグなのだが... //
	if(Viv.credit){
		// クレジットの残っている場合(コンティニュー Y/N 処理へ) //
		Viv.credit -= 1;
	}
}

void GameProc(bool&)
{
#ifdef PBG_DEBUG
	if(DebugDat.DemoSave && DemoplayRecord(Key_Data)) {
		DemoplaySaveDemo();
	}
#endif

	// リプレイ時の保存処理 //
	if(ConfigDat.StageSelect.v && DemoplayRecord(Key_Data)) {
		DemoplaySaveReplay();
	}

	if(Key_Data & KEY_ESC){
		if(ConfigDat.StageSelect.v) {
			DemoplaySaveReplay();
			GameExit(TRUE);
			return;
		}

		ExitWindow.Open({ 250, 150 }, 1);
		GameMain = PauseProc;
		return;
	}
/*
	static BYTE count;
	if(count) count--;
	if((Key_Data & KEY_TAMA) && count==0){
		CEffectSet(Viv.x,Viv.y,CEFC_CIRCLE2);//STAR);
		count = 30;
	}
	if((Key_Data & KEY_BOMB) && count==0){
		CEffectSet(Viv.x,Viv.y,CEFC_CIRCLE1);//STAR);
		count = 30;
	}
*/
	GameMove();
	if(GameMain != GameProc) return;

	if(IsDraw()){
		GameDraw();
		if(DemoplaySaveEnable){
			PIXEL_LTRB rc = PIXEL_LTWH{ 288, 80, 24, 8 };
			GrpBlt(&rc,128,470,GrTama);
		}
		GrpFlip();
	}
}

// ゲームオーバー出現用
void GameOverProc0(bool&)
{
	switch(GameOverTimer) {
	default:
		GameOverTimer--;
		fragment_move();
		SEffectMove();
		break;

	case 0:
		// Wait for press
		if(Key_Data != 0) {
			GameOverTimer--;
		}
		break;

	case -1:
		// Wait for release
		if(Key_Data != 0) {
			break;
		}

		// ステージセレクトが有効な場合 //
		if(ConfigDat.StageSelect.v) {
			DemoplaySaveReplay();
			if(GameStage == GRAPH_ID_EXSTAGE) NameRegistInit(TRUE);
			else                              GameExit(TRUE);
			return;
		}

		if(Viv.credit == 0){
			NameRegistInit(TRUE);
			//GameExit();
			return;			// 仮
		}

		ContinueWindow.Open({ 250, 200 }, 0);
		GameMain = GameOverProc;
		return;
	}

	if(IsDraw()){
		GameDraw();
		GrpFlip();
	}
}

// ゲームオーバー
void GameOverProc(bool&)
{
	CWinMove(&ContinueWindow);
	if(GameMain != GameOverProc){
		SEffectInit();
		return;
	}

	if(IsDraw()){
		GameDraw();
		CWinDraw(&ContinueWindow);
		/*
		if(DemoplaySaveEnable){
			PIXEL_LTRB rc = PIXEL_LTWH{ 288, 80, 24, 8 };
			GrpBlt(&rc,128,470,GrTama);
		}*/
		GrpFlip();
	}
}

// デモプレイ
void DemoProc(bool&)
{
	static BYTE ExTimer;

	ExTimer = (ExTimer+1)%128;

	if(Key_Data) Key_Data = KEY_ESC;
	else         Key_Data = DemoplayMove();

	IsDemoplay = TRUE;

	// ＥＳＣが押されたら即、終了 //
	if(Key_Data & KEY_ESC){
		DemoplayCleanup();
		IsDemoplay = FALSE;
		GameExit();
		return;
	}

	GameMove();

	if(GameMain != DemoProc){
		DemoplayCleanup();	// 後始末
		IsDemoplay = FALSE;
		GameExit();					// 強制終了させる(ゲームオーバー対策)
		return;
	}

	if(IsDraw()){
		GameDraw();
		if(ExTimer<64) GrpPut16(200,200,"D E M O   P L A Y");
		GrpFlip();
	}
}

// 装備選択 //
void WeaponSelectProc(bool&)
{
	PIXEL_LTRB	rc;
	int		i,x,y;
	BYTE	d;
	char	buf[100];

	static char deg  = 0;
	static char spd  = 0;
	static WORD	count = 0;

	PIXEL_LTRB src[4] = {
		PIXEL_LTWH{  0, 344, 56, 48 },
		PIXEL_LTWH{  0, 392, 56, 48 },
		PIXEL_LTWH{ 56, 344, 56, 48 },
		PIXEL_LTWH{ 56, 392, 56, 48 },
	};

	deg += spd;
	if(deg>=85 || deg<=-85){
	//if(deg>=64 || deg<=-64){
		//if(spd<0) Viv.weapon = (Viv.weapon+3)%4;
		//else      Viv.weapon = (Viv.weapon+1)%4;
		if(spd<0) Viv.weapon = (Viv.weapon+2)%3;
		else      Viv.weapon = (Viv.weapon+1)%3;
		spd = 0;
		deg = 0;
		Snd_SEPlay(SOUND_ID_BUZZ);
	}

	if(WeaponKeyWait){
		if(!Key_Data) WeaponKeyWait = 0;
		else          Key_Data      = 0;
	}

	switch(Key_Data){
		case(KEY_RIGHT):
			if(spd<0){
				//Viv.weapon = (Viv.weapon+3)%4;
				//deg+=64;
				Viv.weapon = (Viv.weapon+2)%3;
				deg+=85;
			}
			spd  = 3;
		break;

		case(KEY_LEFT):
			if(spd>0){
				//Viv.weapon = (Viv.weapon+1)%4;
				//deg-=64;
				Viv.weapon = (Viv.weapon+1)%3;
				deg-=85;
			}
			spd  = -3;
		break;

		case(KEY_TAMA):case(KEY_RETURN):
			if(spd) break;
			if(GameStage == GRAPH_ID_EXSTAGE){
				if(!((1 << Viv.weapon) & ConfigDat.ExtraStgFlags.v)) {
					break;
				}
			}

			VivTemp.weapon = Viv.weapon;
			Viv = VivTemp;
			MaidTamaIndSet();
			count = 0;

			Snd_SEPlay(SOUND_ID_SELECT);
			if(GameStage != GRAPH_ID_EXSTAGE){
			#ifdef PBG_DEBUG
				GameStage = DebugDat.StgSelect;
				if(GameStage==2) Viv.exp = 160;
				if(GameStage>=3) Viv.exp = 255;
				if(DebugDat.DemoSave) DemoplayInit();
			#else
				// リプレイ用の処理を追加 //
				if(ConfigDat.StageSelect.v) {
					GameStage = ConfigDat.StageSelect.v;
					if(GameStage==2) Viv.exp = 160;
					if(GameStage>=3) Viv.exp = 255;
					DemoplayInit();
				}
				else{
					GameStage = 1;
				}
			#endif
			}
			else{
				Viv.credit = 0;
				Viv.left   = 2;
				Viv.exp    = 255;
				if(ConfigDat.StageSelect.v) {
					DemoplayInit();
				}
			}

			if(!LoadGraph(GameStage)){
				DebugOut(u8"GRAPH.DAT が破壊されています");
				return;
			}
			if(!LoadStageData(GameStage)){
				DebugOut(u8"ENEMY.DAT が破壊されています");
				return;
			}

			GameInit(GameProc);
		return;

		case(KEY_ESC):case(KEY_BOMB):
			if(spd) break;
			Snd_SEPlay(SOUND_ID_CANCEL);
			GameExit(FALSE);
		return;
	}

	count = (count+1)%(256+128);

	if(IsDraw()){
		GrpCls();

		rc = { 0, (264 - 8), 224, (296 - 24) };
		GrpBlt(&rc,320-112,20,GrTama);

		rc = PIXEL_LTWH{ 0, 272, 64, 24 };
		GrpBlt(&rc,120-32,260-12,GrTama);

		d = ((count/8)%2)<<3;
		rc = PIXEL_LTWH{ 72, (272 + d), 56, 8 };
		GrpBlt(&rc,400-28+4,420,GrTama);

		for(i=0;i<3;i++){
		//for(i=0;i<4;i++){
			//d = (-i+Viv.weapon)*64 + deg - 64;
			d = (-i+Viv.weapon)*85 + deg - 64;
			x = 120 + cosl(d,90) - 56/2;
			y = 260 + sinl(d,110) - 48/2;
			GrpBlt(&src[i], x, y, GrTama);
		}

		GrpLock();
		GrpSetColor(0,0,1);
		GrpSetAlpha(128, ALPHA_NORM);
		for(i=0;i<3;i++){
			if(
				(GameStage != GRAPH_ID_EXSTAGE) ||
				((1 << i) & ConfigDat.ExtraStgFlags.v)
			) {
				continue;
			}

			d = (-i+Viv.weapon)*85 + deg - 64;
			x = 120 + cosl(d,90) - 56/2;
			y = 260 + sinl(d,110) - 48/2;
			GrpBoxA(x, y, x+56, y+48);
		}
		GrpUnlock();

		Viv.exp = min(count,255);
		if(Viv.exp < 31) Viv.lay_time = Viv.lay_grp =  0;

		HomingFlag=HOMING_DUMMY;
		Key_Data = KEY_TAMA;

		Viv.muteki = 0;
		Viv.x = 400*64 + sinl((count/3)*6,60*64);
		Viv.y = 350*64 + sinl((count/3)*4,30*64);

		MaidMove();
		MaidTamaMove();

		GrpSetClip(400-110,400-300+2,400+110,400+10);
		for(x=400-110-2; x<400+110; x+=32){
			for(y=400-300+2+(count*2)%32-32; y<400+10; y+=32){
				d = Viv.weapon<<4;
				rc = PIXEL_LTWH{ 224, 256, 32, 32 };
				//rc = PIXEL_LTWH{ d, (296 - 24), 16, 16 };
				GrpBlt(&rc,x,y,GrTama);
			}
		}
		MaidDraw();
		MaidTamaDraw();

		rc = PIXEL_LTWH{ 72, (272 + 16), 56, 8 };
		GrpBlt(&rc,468,400,GrTama);
		sprintf(buf,"%d",((WORD)Viv.exp+1)>>5);
		GrpPutScore(500,400,buf);

		GrpLock();
		GrpSetClip(0,0,640,480);

		GrpSetColor(0,0,4);
		GrpLine(400-110,400-300,400+110,400-300);
		GrpLine(400-110,400+10,400+110,400+10);
		GrpLine(400-110,400-300,400-110,400+10);
		GrpLine(400+110,400-300,400+110,400+10);

		if(abs((char)deg)<=25){
			GrpSetColor(2,2,5);
			GrpCircle(120,150,39+10-2*abs((char)deg));
			GrpSetColor(4,4,5);
			GrpCircle(120,150,41+10-2*abs((char)deg));
		}
		GrpUnlock();

/*
		HDC		hdc;
		char	buf[100];
		DxObj.Back->GetDC(&hdc);
		sprintf(buf,"Viv.weapon = %d",Viv.weapon);
		TextOut(hdc,0,0,buf,strlen(buf));
		DxObj.Back->ReleaseDC(hdc);

		#ifdef PBG_DEBUG
			StdStatusOutput();
		#endif
*/
		GrpFlip();
	}
}


void TitleProc(bool& quit)
{
	PIXEL_LTRB	src = { 0, 0, 640, 396 };
	//PIXEL_LTRB	src = { 0, 0, 350, 403 };
	//PIXEL_LTRB	src = { 0, 0, 195, 256 };
	//PIXEL_LTRB	src = { 0, 0, 275, 256 };

/*
	// 鳩プロテクト? //
	if(
		(GetAsyncKeyState(VK_F1) & 0x80000000) &&
		(GetAsyncKeyState(VK_F10) & 0x8000000)
	) {
		quit = true;
	}
*/
	// Running this here to prevent MIDI processing from jumping over a large
	// number of events once the player enters the Music Room.
	BGM_UpdateMIDITables();

	if(Key_Data==0) DemoTimer += 1;
	else            DemoTimer  = 0;
	if(MainWindow.SelectDepth!=0) DemoTimer = 0;

	if(DemoTimer==60*10){	// 60*3
		DemoInit();
		return;
	}

	auto* window_active = ((BGMPackWindow.State != CWIN_DEAD)
		? &BGMPackWindow
		: &MainWindow
	);
	CWinMove(window_active);
	MWinHelp(window_active);
	MWinMove();
	if(GameMain!=TitleProc) return;

	if(MainWindow.State==CWIN_DEAD){
		switch(MainWindow.Select[0]){
			case(0):
				WeaponSelectInit(false);
			return;

			default:
				quit = true;
			return;
		}
	}

	if(IsDraw()){
		GrpCls();
		GrpBlt(&src, 0, 42, GrTitle);
		// GrpBlt(&src, (320 - 175), 77, GrTitle);
		MWinDraw();
		CWinDraw(window_active);
		#ifdef PBG_DEBUG
			StdStatusOutput();
		#endif

		GrpFlip();
	}
}

void PauseProc(bool&)
{
	WINDOW_LTRB	rc;

	CWinMove(&ExitWindow);
	if(GameMain!=PauseProc) return;

	if(IsDraw()){
		GameDraw();

		GrpGetClip(&rc);
		GrpSetClip(0,0,640,480);
		CWinDraw(&ExitWindow);
		GrpSetClip(rc.left, rc.top, rc.right, rc.bottom);

		GrpFlip();
	}
}

// 音楽室 //
void MusicRoomProc(bool&)
{
	char	buf[100];
	static WORD Old_Key;
	static BYTE DevChgWait;

	const auto playing = BGM_Playing();

	if(Key_Data!=Old_Key){
		switch(Key_Data){
			case(KEY_ESC):case(KEY_BOMB):
				DevChgWait = 0;
				GameExit();
			return;

			case(KEY_RIGHT):
				MidiPlayID += 2; [[fallthrough]];
			case(KEY_LEFT):
				BGM_Stop();
				MidiPlayID = (MidiPlayID+MusicNum-1)%MusicNum;
				BGM_Switch(MidiPlayID);
				LoadMusicRoomComment(MidiPlayID);
			break;
		}

		Old_Key = Key_Data;
	}

	switch(Key_Data){
		case(KEY_UP):
			BGM_SetTempo(BGM_GetTempo() + 1);
		break;
		case(KEY_DOWN):
			BGM_SetTempo(BGM_GetTempo() - 1);
		break;

		case(KEY_SHIFT):
			BGM_SetTempo(0);
		break;
	}

	if(SystemKey_Data & SYSKEY_BGM_FADE) {
		BGM_FadeOut(120);
	}

	BGM_UpdateMIDITables();

	if((playing == BGM_PLAYING::MIDI) && (SystemKey_Data & SYSKEY_BGM_DEVICE)) {
		if(DevChgWait==0){
			BGM_ChangeMIDIDevice(1);
			if(MusicRoomText) {
				MusicRoomText.value().PrerenderMidDev();
			}
			DevChgWait = 1;
		}
	}
	else{
		// 押されていなければ再び有効化する //
		DevChgWait = 0;
	}

	if(IsDraw()){
		GrpCls();

		auto BlitBG = [](const PIXEL_LTWH& rect) {
			PIXEL_LTRB src = rect;
			GrpBlt(&src, src.left, src.top, GrMusic);
		};

		auto BlitLegend = [](const PIXEL_LTWH& rect) {
			PIXEL_LTRB src = (rect + PIXEL_POINT{ 0, 392 });
			GrpBlt(&src, (8 + rect.left), (410 + rect.top), GrMusic);
		};

		BlitBG({   0,   0, 504, 392 });	// From keyboard to spectrum analyzer
		BlitBG({ 504,   0, 136,  59 });	// Down to PASSED TIME
		BlitBG({ 504, 108, 136, 284 });	// Everything below MIDI DEVICE

		BlitLegend({   0,  0, 176,  40 });	// Left side
		BlitLegend({ 176, 11, 464,  29 });	// Right side without device key

		if(playing == BGM_PLAYING::MIDI) {
			BlitBG({ 504, 83, 136, 25 });   	// MIDI DEVICE
			BlitLegend({ 176, 0, 176, 11 });	// Device change key
		}

		//GrpDrawSpect(0,480);
		GrpDrawSpect(352,128);
		GrpDrawNote();

		const auto millis = BGM_PlayTime().count();
		const auto m = ((millis / 1000) / 60);
		const auto s = ((millis / 1000) % 60);
		sprintf(buf,"%02d : %02d",m,s);
		GrpPut7B(560,40+2+2,buf);
		//TextOut(hdc,561,40+2,buf,strlen(buf));

		if(Mid_Loaded()) {
			BlitBG({ 504,  59, 136,  24 });	// MIDI TIMER
			sprintf(buf, "%07lld", Mid_PlayTime.pulse_interpolated);
			GrpPut7B(560,64+2+2,buf);
			//TextOut(hdc,561,64+2,buf,strlen(buf));
		}

		sprintf(buf, "%3d", BGM_GetTempo());
		GrpPut7B(560,112+2+2,buf);
		//TextOut(hdc,561,112+2,buf,strlen(buf));
		//SetTextColor(hdc,RGB(255*5/5,255*2/5,255*1/5));

		if(MusicRoomText) {
			const auto text = MusicRoomText.value();

			if(playing == BGM_PLAYING::MIDI) {
				TextObj.Blit({ (540 + 2), (96 - 3) }, text.mid_dev);
			}
			TextObj.Blit({  400,       (144 +  2) }, text.title);
			TextObj.Blit({ (400 - 40), (144 + 30) }, text.comment);
			TextObj.Blit({ (200 - 50),  460       }, text.version);
		}

		GrpFlip();
	}
}

/*
_inline XAdd(DWORD old,int id)
{
	RndBuf[id] += (random_ref-old);
}
*/


void GameMove(void)
{
	MWinMove();

	ScrollMove();

	BossMove();
	enemy_move();
	ItemMove();
	tama_move();
	laser_move();
	LLaserMove();
	HLaserMove();
	fragment_move();
	SEffectMove();
	CEffectMove();
	ExBombEfcMove();
	ObjectLockMove();

	WarningEffectMove();
	ScreenEffectMove();

	// この２行の位置を変更しました //
	MaidMove();
	MaidTamaMove();
}


void GameDraw(void)
{
//	WINDOW_LTRB	rc;


	GrpCls();

	ScrollDraw();
	CEffectDraw();

	BossDraw();

	WideBombDraw();		// 多分、ここで良いと思うが...

	ExBombEfcDraw();

	enemy_draw();

	MaidTamaDraw();

	MaidDraw();

	if(DxObj.PixelFormat.IsPalettized()) LLaserDraw();

	ObjectLockDraw();

	fragment_draw();
	ItemDraw();

	if(DxObj.PixelFormat.IsChanneled()) LLaserDraw();

	HLaserDraw();
	laser_draw();
	tama_draw();

	// static uint8_t test = 0;

	//if((Key_Data&KEY_UP  ) && test<64) test++;
	//if((Key_Data&KEY_DOWN) && test!=0 ) test--;
	WarningEffectDraw();
	//MoveWarning(test++);
	//DrawWarning();

	SEffectDraw();
	StateDraw();

	BossHPG_Draw();
	ScreenEffectDraw();

//	GrpGetClip(&rc);
//	GrpSetClip(0,0,640,480);
	MWinDraw();
//	GrpSetClip(rc.left, rc.top, rc.right, rc.bottom);

	StdStatusOutput();
}

bool IsDraw(void)
{
	if(ConfigDat.FPSDivisor.v) {
		DrawCount++;
		if(DrawCount % ConfigDat.FPSDivisor.v) {
			return false;
		}
	}

	return TRUE;
}
