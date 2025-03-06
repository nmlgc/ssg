/*                                                                           */
/*   GameMain.cpp   ウィンドウシステム切り替えなどの処理                     */
/*                                                                           */
/*                                                                           */

#include "GAMEMAIN.H"
#include "BOMBEFC.H" // 爆発エフェクト処理
#include "CONFIG.H"
#include "DEMOPLAY.H"
#include "FONTUTY.H"
#include "GEOMETRY.H"
#include "GIAN.H"
#include "LENS.H"
#include "LEVEL.H"
#include "MUSIC.H"
#include "SCORE.H"
#include "WindowCtrl.h" // ウィンドウ定義
#include "WindowSys.h"
#include "platform/text_backend.h"
#include "game/bgm.h"
#include "game/debug.h"
#include "game/input.h"
#include "game/snd.h"
#include "game/ut_math.h"
#include "platform/time.h"


uint16_t DemoTimer = 0;
uint32_t DrawCount = 0;
uint8_t WeaponKeyWait = 0;
int	GameOverTimer = 0;

NR_NAME_DATA	CurrentName;	// ネームレジスト準備用データ
uint8_t	CurrentRank;	// ネームレジスト用順位データ
uint8_t	CurrentDif;	// 現在の難易度(スコアネーム表示用)

MAID		VivTemp;

bool IsDemoplay = false;


void TitleProc(bool& quit);
void WeaponSelectProc(bool&);	// 装備選択
void GameProc(bool&);
void GameOverProc0(bool&);	// ゲームオーバー出現用
void GameOverProc(bool&);	// ゲームオーバー
void PauseProc(bool&);
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

	CurrentRank = SetScoreString(nullptr, CurrentDif);
	if(CurrentRank == 0) return GameExit();

	MWinForceClose();
	GrpBackend_Clear();
	Grp_Flip();

	if(!LoadGraph(GRAPH_ID_NAMEREGIST)){
		DebugOut(u8"GRAPH.DAT が破壊されています");
		return false;
	}

	GrpBackend_SetClip(GRP_RES_RECT);

	InputLocked = Key_Data;
	GameMain = ScoreNameProc;

	return true;
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
			GameExit(false);
		return;

		case(KEY_UP):case(KEY_LEFT):
			if(ScoreString[4].bMoveEnable) break;
			Snd_SEPlay(SOUND_ID_SELECT);
			CurrentDif  = (CurrentDif+4)%5;
			CurrentRank = SetScoreString(nullptr, CurrentDif);
		break;

		case(KEY_DOWN):		case(KEY_RIGHT):
			if(ScoreString[4].bMoveEnable) break;
			Snd_SEPlay(SOUND_ID_SELECT);
			CurrentDif  = (CurrentDif+1)%5;
			CurrentRank = SetScoreString(nullptr, CurrentDif);
		break;

		case 0:
			InputLocked = false;
			break;
	}

	GrpBackend_Clear();
	ScoreDraw();
	GrpPut16(320,450,ExString[CurrentDif]);
	Grp_Flip();
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
		} else {
			ScoreString[i].bMoveEnable = false;
		}

		src = PIXEL_LTWH{ 0, (64 + (32 * i)), 400, 32 };
		const WINDOW_POINT topleft = {
			(ScoreString[i].x >> 6),
			(ScoreString[i].y >> 6),
		};
		GrpSurface_Blit(topleft, SURFACE_ID::NAMEREG, src);
		// GrpSurface_Blit(
		// 	{ (50 + (i * 24)), (100 + (i * 48) }, SURFACE_ID::NAMEREG, src
		// );

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
			GrpSurface_Blit({ gx, (gy - 1) }, SURFACE_ID::SYSTEM, src);
		}
		else
			GrpPutScore(gx, gy, ScoreString[i].Stage);

		gx = (ScoreString[i].x>>6) + 224 + 80;
		gy = (ScoreString[i].y>>6) + 25;
		src = PIXEL_LTWH{ 0, (400 + (ScoreString[i].Weapon * 8)), 48, 8 };
		GrpSurface_Blit({ gx, (gy - 1) }, SURFACE_ID::NAMEREG, src);
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
		return ('A' + (x % 26));
	}
	// 小文字 //
	else if (y == 1){
		return ('a' + (x % 26));
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
	static uint8_t count;
	static uint8_t time;
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

	GrpBackend_Clear();

	GrpGeom->Lock();

	GrpGeom->SetColor({ 2, 0, 0 });
	gx = ScoreString[CurrentRank-1].x >> 6;
	gy = ScoreString[CurrentRank-1].y >> 6;
	GrpGeom->DrawBox(gx, gy, (gx + 400), (gy + 32));

	if(time%64 > 32){
		GrpGeom->SetColor({ 4, 0, 0 });
		len = min(strlen(ScoreString[CurrentRank-1].Name), NR_NAME_LEN-2);
		gx += (len*16 + 88);
		gy += (4);
		GrpGeom->DrawBox(gx, gy, (gx + 14), (gy + 16));
	}

	GrpGeom->Unlock();

	constexpr auto sid = SURFACE_ID::NAMEREG;
	GrpSurface_Blit({ 120, 0 }, sid, src);

	ScoreDraw();

	// 名前入力用文字列群 //
	src = { 0, 432, 416, 480 };
	GrpSurface_Blit({ 112, 420 }, sid, src);

	// カーソル //
	if((x >= 20) && (y == 2)) {
		src = PIXEL_LTWH{ 432, (432 + ((count >> 3) << 4)), 32, 16 };
	} else {
		src = PIXEL_LTWH{ 416, (432 + ((count >> 3) << 4)), 16, 16 };
	}
	GrpSurface_Blit({ (112 + (x << 4)), (420 + (y << 4)) }, sid, src);

	//sprintf(buf,"(%2d,%2d)", x, y);
	//GrpPut16(0,0,buf);

/*
	GrpPut16(400,100,temps);
	for(i=0; i<5; i++){
		GrpPut16(100, 100+i*32, ScoreString[i].Score);
		if(CurrentRank == i+1) GrpPut16(85, 100+i*32, "!!");
	}
*/
	Grp_Flip();
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
	CurrentRank = SetScoreString(&CurrentName, CurrentLevel());
	if(CurrentRank == 0) return GameExit();

	MWinForceClose();
	GrpBackend_Clear();
	Grp_Flip();

	if(!LoadGraph(GRAPH_ID_NAMEREGIST)){
		DebugOut(u8"GRAPH.DAT が破壊されています");
		return false;
	}

	GrpBackend_SetClip(GRP_RES_RECT);

	InputLocked = Key_Data;
	GameMain = NameRegistProc;

	if(bNeedChgMusic){
		BGM_Switch(19);
	}

	return true;
}

// ゲームを立ち上げる際に必ず行う初期化関数群 //
void GameSTD_Init(void)
{
	MWinForceClose();
	// GrpBackend_Clear();
	// Grp_Flip();

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
	GrpBackend_Clear();
	Grp_Flip();

	GameLevel = (ExStg ? GAME_HARD : ConfigDat.GameLevel.v);

	GameSTD_Init();
	PlayRankReset();

	MaidSet();

	GrpBackend_SetClip(GRP_RES_RECT);

	WeaponKeyWait = 1;
	Viv.weapon    = 0;
	GameMain = WeaponSelectProc;
	if(ExStg) {
		GameStage = GRAPH_ID_EXSTAGE;
	}

	VivTemp = Viv;

	return true;
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
	GrpBackend_SetClip(PLAYFIELD_CLIP);
	GameMain = NextProc;
	return true;
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
		return false;
	}
	if(!LoadStageData(GameStage)){
		DebugOut(u8"ENEMY.DAT が破壊されています");
		return false;
	}

	return true;
}


// リプレイ用の初期化を行う //
extern bool GameReplayInit(int Stage)
{
	MaidSet();

//	rnd_seed_set(Time_SteadyTicksMS());
	GameStage = Stage;

	if(!DemoplayLoadReplay(GameStage)) {
		// DebugOut(u8"デモプレイデータが存在せず");
		return false;
	}

	PlayRankReset();

	GrpBackend_Clear();
	Grp_Flip();
	GameSTD_Init();

	if(!LoadGraph(GameStage)){
		DebugOut(u8"GRAPH.DAT が破壊されています");
		DemoplayCleanup();
		return false;
	}
	if(!LoadStageData(GameStage)){
		DebugOut(u8"ENEMY.DAT が破壊されています");
		DemoplayCleanup();
		return false;
	}

	if(GameStage == GRAPH_ID_EXSTAGE){
		Viv.credit = 0;
	}

	return GameInit(ReplayProc);
}

void ReplayProc(bool&)
{
	static uint8_t ExTimer = 0;

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

		constexpr PIXEL_LTWH rc = { 312, 80, 32, 8 };
		GrpSurface_Blit({ 128, 470 }, SURFACE_ID::SYSTEM, rc);
		if(ExTimer<64+32){
			GrpGeom->Lock();
			GrpGeom->SetAlphaNorm(128);
			GrpGeom->SetColor({ 0, 0, 0 });
			GrpGeom->DrawBoxA(
				(128 + 45 - 3), (470 + 4 - 1), (128 + 45 + 72), (470 + 4 + 5)
			);
			GrpGeom->Unlock();
			constexpr PIXEL_LTWH rc = { 312, 88, 72, 8 };
			GrpSurface_Blit({ (128 + 45), (470 + 4) }, SURFACE_ID::SYSTEM, rc);
		}
		Grp_Flip();
	}
}


// デモプレイの初期化を行う //
bool DemoInit(void)
{
//	return NameRegistInit();

	GrpBackend_Clear();
	Grp_Flip();

	GameSTD_Init();

	MaidSet();

	rnd_seed_set(Time_SteadyTicksMS());
	GameStage = (rnd()%STAGE_MAX)+1;

//	GameStage = (rnd()%3)+1;		// この部分は体験版(コミケ)だけ //
//	GameStage = 1;					// こっちはＨＰ体験版用         //

	if(!DemoplayLoadDemo(GameStage)) {
		// DebugOut(u8"デモプレイデータが存在せず");
		return false;
	}

	PlayRankReset();

	if(!LoadGraph(GameStage)){
		DebugOut(u8"GRAPH.DAT が破壊されています");
		return false;
	}
	if(!LoadStageData(GameStage)){
		DebugOut(u8"ENEMY.DAT が破壊されています");
		return false;
	}

	return GameInit(DemoProc);
}

// 西方Ｐｒｏｊｅｃｔ表示動作部 //
std::optional<LensInfo> Lens;

void SProjectProc(bool&)
{
	static uint16_t timer = 0;

	constexpr PIXEL_SIZE logo_size = { 320, 42 };
	constexpr WINDOW_LTRB logo = WINDOW_LTWH{
		(320 - (logo_size.w / 2)), (240 + 40), logo_size.w, logo_size.h
	};

	const auto fade = [logo](uint8_t black_alpha, uint8_t palette_tone) {
		if(auto *gp = GrpGeom_Poly()) {
			gp->Lock();
			gp->SetAlphaNorm(black_alpha);
			gp->SetColor({ 0, 0, 0 });
			gp->DrawBoxA(logo.left, logo.top, logo.right, logo.bottom);
			gp->Unlock();
		} else {
			GrpBackend_PaletteSet(SProjectPalette.Fade(palette_tone));
		}
	};

	constexpr PIXEL_LTRB rc = { 0, 0, logo_size.w, logo_size.h };
	int		x, y;

	timer = timer + 1;

	if(timer >= 256){
		timer   = 0;

		Lens = std::nullopt;
		GameExit();
		return;
	}

	if(IsDraw()){
		GrpBackend_Clear(/* 255 */);

		GrpSurface_Blit({ logo.left, logo.top }, SURFACE_ID::SPROJECT, rc);

		if(timer < 64){
			fade(((255 - timer) * 4), (timer * 4));
		}
		else if(timer > 192){
			fade((timer * 4), ((255 - timer) * 4));
		}
		else if(Lens) {
			const uint8_t d = (timer - 64);
			x = 320 + sinl(d-64, 240);
			y = 295 + sinl(d*2, 20);
			Lens.value().Draw({ x, y });
		}

		Grp_Flip();
	}
}


// 西方Ｐｒｏｊｅｃｔ表示の初期化 //
bool SProjectInit(void)
{
	GrpBackend_PixelAccessStart();

	if(!LoadGraph(GRAPH_ID_SPROJECT)){
		DebugOut(u8"GRAPH.DAT が破壊されています");
		return false;
	}

	// レンズをすでに作成しているのなら、破棄する //
	Lens = GrpCreateLensBall(70, 36);
	if(!Lens) {
		return false;
	}

	GameMain = SProjectProc;

	return true;
}


// ゲームを再開する(ESC 抜けから) //
extern void GameRestart(void)
{
	GameMain = GameProc;
}

// ゲームから抜ける //
extern bool GameExit(bool bNeedChgMusic)
{
	GrpBackend_PixelAccessEnd();
	TextObj.Clear();
	GrpBackend_Clear();
	Grp_Flip();

	if(!LoadGraph(GRAPH_ID_TITLE)){
		DebugOut(u8"GRAPH.DAT が破壊されています");
		return false;
	}
	GrpBackend_SetClip(GRP_RES_RECT);

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

	return true;
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
			GameExit(true);
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
			constexpr PIXEL_LTRB rc = PIXEL_LTWH{ 288, 80, 24, 8 };
			GrpSurface_Blit({ 128, 470 }, SURFACE_ID::SYSTEM, rc);
		}
		Grp_Flip();
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
			if(GameStage == GRAPH_ID_EXSTAGE) {
				NameRegistInit(true);
			} else {
				GameExit(true);
			}
			return;
		}

		if(Viv.credit == 0){
			NameRegistInit(true);
			//GameExit();
			return;			// 仮
		}

		ContinueWindow.Open({ 250, 200 }, 0);
		GameMain = GameOverProc;
		return;
	}

	if(IsDraw()){
		GameDraw();
		Grp_Flip();
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
			constexpr PIXEL_LTRB rc = PIXEL_LTWH{ 288, 80, 24, 8 };
			GrpSurface_Blit({ 128, 470 }, SURFACE_ID::SYSTEM, rc);
		}*/
		Grp_Flip();
	}
}

// デモプレイ
void DemoProc(bool&)
{
	static uint8_t ExTimer = 0;

	ExTimer = (ExTimer+1)%128;

	if(Key_Data) Key_Data = KEY_ESC;
	else         Key_Data = DemoplayMove();

	IsDemoplay = true;

	// ＥＳＣが押されたら即、終了 //
	if(Key_Data & KEY_ESC){
		DemoplayCleanup();
		IsDemoplay = false;
		GameExit();
		return;
	}

	GameMove();

	if(GameMain != DemoProc){
		DemoplayCleanup();	// 後始末
		IsDemoplay = false;
		GameExit();					// 強制終了させる(ゲームオーバー対策)
		return;
	}

	if(IsDraw()){
		GameDraw();
		if(ExTimer<64) GrpPut16(200,200,"D E M O   P L A Y");
		Grp_Flip();
	}
}

// 装備選択 //
void WeaponSelectProc(bool&)
{
	PIXEL_LTRB	rc;
	int		i,x,y;
	char	buf[100];

	static char deg  = 0;
	static char spd  = 0;
	static uint16_t count = 0;

	constexpr PIXEL_LTRB src[4] = {
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
			GameExit(false);
		return;
	}

	count = (count+1)%(256+128);

	if(IsDraw()){
		GrpBackend_Clear();

		rc = { 0, (264 - 8), 224, (296 - 24) };
		GrpSurface_Blit({ (320 - 112), 20 }, SURFACE_ID::SYSTEM, rc);

		rc = PIXEL_LTWH{ 0, 272, 64, 24 };
		GrpSurface_Blit({ (120 - 32), (260 - 12) }, SURFACE_ID::SYSTEM, rc);

		uint8_t d = (((count / 8) % 2) << 3);
		rc = PIXEL_LTWH{ 72, (272 + d), 56, 8 };
		GrpSurface_Blit({ (400 - 28 + 4), 420 }, SURFACE_ID::SYSTEM, rc);

		for(i=0;i<3;i++){
		//for(i=0;i<4;i++){
			//d = (-i+Viv.weapon)*64 + deg - 64;
			d = (-i+Viv.weapon)*85 + deg - 64;
			x = 120 + cosl(d,90) - 56/2;
			y = 260 + sinl(d,110) - 48/2;
			GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src[i]);
		}

		GrpGeom->Lock();
		GrpGeom->SetColor({ 0, 0, 1 });
		GrpGeom->SetAlphaNorm(128);
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
			GrpGeom->DrawBoxA(x, y, (x + 56), (y + 48));
		}
		GrpGeom->Unlock();

		Viv.exp = min(count,255);
		if(Viv.exp < 31) Viv.lay_time = Viv.lay_grp =  0;

		HomingFlag=HOMING_DUMMY;
		Key_Data = KEY_TAMA;

		Viv.muteki = 0;
		Viv.x = 400*64 + sinl((count/3)*6,60*64);
		Viv.y = 350*64 + sinl((count/3)*4,30*64);

		MaidMove();
		MaidTamaMove();

		GrpBackend_SetClip(
			{ (400 - 110), (400 - 300 + 2), (400 + 110), (400 + 10) }
		);
		for(x=400-110-2; x<400+110; x+=32){
			for(y=400-300+2+(count*2)%32-32; y<400+10; y+=32){
				d = Viv.weapon<<4;
				rc = PIXEL_LTWH{ 224, 256, 32, 32 };
				//rc = PIXEL_LTWH{ d, (296 - 24), 16, 16 };
				GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, rc);
			}
		}
		MaidDraw();
		MaidTamaDraw();

		rc = PIXEL_LTWH{ 72, (272 + 16), 56, 8 };
		GrpSurface_Blit({ 468, 400 }, SURFACE_ID::SYSTEM, rc);
		sprintf(buf, "%d", ((Cast::up<uint16_t>(Viv.exp) + 1) >> 5));
		GrpPutScore(500,400,buf);

		GrpBackend_SetClip(GRP_RES_RECT);

		GrpGeom->Lock();
		GrpGeom->SetColor({ 0, 0, 4 });
		GrpGeom->DrawLine((400 - 110), (400 - 300), (400 + 110), (400 - 300));
		GrpGeom->DrawLine((400 - 110), (400 +  10), (400 + 110), (400 +  10));
		GrpGeom->DrawLine((400 - 110), (400 - 300), (400 - 110), (400 +  10));
		GrpGeom->DrawLine((400 + 110), (400 - 300), (400 + 110), (400 +  10));

		if(abs(deg) <= 25) {
			GrpGeom->SetColor({ 2, 2, 5 });
			GeomCircle({ 120, 150 }, (39 + 10 - (2 * abs(deg))));
			GrpGeom->SetColor({ 4, 4, 5 });
			GeomCircle({ 120, 150 }, (41 + 10 - (2 * abs(deg))));
		}
		GrpGeom->Unlock();

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
		Grp_Flip();
	}
}


void TitleProc(bool& quit)
{
	constexpr PIXEL_LTRB src = { 0, 0, 640, 396 };
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
		GrpBackend_Clear();
		GrpSurface_Blit({ 0, 42 }, SURFACE_ID::TITLE, src);
		// GrpSurface_Blit({ (320 - 175), 77 }, SURFACE_ID::TITLE, src);
		MWinDraw();
		CWinDraw(window_active);
#ifdef PBG_DEBUG
		StdStatusOutput();
#endif

		Grp_Flip();
	}
}

void PauseProc(bool&)
{
	CWinMove(&ExitWindow);
	if(GameMain!=PauseProc) return;

	if(IsDraw()){
		GameDraw();

		GrpBackend_SetClip(GRP_RES_RECT);
		CWinDraw(&ExitWindow);
		GrpBackend_SetClip(PLAYFIELD_CLIP);

		Grp_Flip();
	}
}

/*
inline XAdd(DWORD old,int id)
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
	GrpBackend_Clear();

	ScrollDraw();
	CEffectDraw();

	BossDraw();

	WideBombDraw();		// 多分、ここで良いと思うが...

	ExBombEfcDraw();

	enemy_draw();

	MaidTamaDraw();

	MaidDraw();

	if(GrpGeom_FB()) {
		LLaserDraw();
	}

	ObjectLockDraw();

	fragment_draw();
	ItemDraw();

	if(GrpGeom_Poly()) {
		LLaserDraw();
	}

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

	MWinDraw();
	// GrpBackend_SetClip(PLAYFIELD_CLIP);

	GrpBackend_SetClip(GRP_RES_RECT);
	StdStatusOutput();
	GrpBackend_SetClip({ X_MIN, Y_MIN, (X_MAX + 1), (Y_MAX + 1) });
}

bool IsDraw(void)
{
	if(Grp_FPSDivisor) {
		DrawCount++;
		if(DrawCount % Grp_FPSDivisor) {
			return false;
		}
	}

	return true;
}
