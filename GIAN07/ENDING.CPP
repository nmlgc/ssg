/*
 *   Ending.cpp   : エンディングの処理
 *
 */

#include "ENDING.H"
#include "FONTUTY.H"
#include "GIAN.H"
#include "SCL.H" // ＳＣＬ定義ファイル
#include "game/bgm.h"
#include "game/cast.h"
#include "game/endian.h"


typedef struct tagEndingGrpInfo {
	uint32_t timer;	// 表示用タイマー
	decltype(timer) fadein;	// フェードイン時刻
	decltype(timer) fadeout;	// フェードアウト時刻
	ENDING_GRP	*target;	// 描画対象
	short		alpha;		// パレットの状態

	int			x, y;		// 表示する左上
	bool	bWantDisp;	// 表示するかどうか
} EndingGrpInfo;

// スタッフ描画タスク //
typedef struct tagEndingStTask{
	uint32_t timer;	// 表示用タイマー
	decltype(timer) fadein;	// フェードイン時刻
	decltype(timer) fadeout;	// フェードアウト時刻

	uint8_t	StfID[10];	// スタッフＩＤ
	uint8_t	TitleID;	// タイトル、すなわち役職ＩＤ

	short	NumStf;	// 全スタッフ数
	short		alpha;		// パレットの状態

	int			ox, oy;		// 表示基準座標

	bool	bWantDisp;	// 表示するのかな
} EndingStTask;

typedef struct tagEndingText {
	Narrow::string_view Text[10];	// 表示するテキストへのポインタ
	int		NumText;		// 現在格納されているテキストの数
	TEXTRENDER_RECT_ID Rect;
	bool Rerender;

	void Prerender();
} EndingText;


EndingGrpInfo	EGrpInfo;
EndingStTask	EStfTask;
EndingText		EText;

const PIXEL_LTRB StaffLabel[7] = {
	{ 0,   0, 160,  24 },
	{ 0,  24, 104,  48 },
	{ 0,  48, 160,  72 },
	{ 0,  72, 232,  96 },
	{ 0,  96, 168, 120 },
	{ 0, 144, 104, 168 },
	{ 0, (480 - 32), (9 * 32), 480 },
};

const PIXEL_LTRB StaffMember[7] = {
	{   0, 168,  72, 192 },
	{  96, 168, 168, 192 },
	{ 192, 168, 264, 192 },
	{ 288, 168, 360, 192 },
	{   0, 192, 144, 216 },
	{ 168, 192, 320, 216 },
	{   0, 216, 336, 264 }
};

// フラッシュの状態 //
uint16_t FlashState = 0;


void UpdateGrpInfo();		// グラフィックの更新(内部データ)
void UpdateStfInfo();		// スタッフの更新(内部データ)

void DrawGrpInfo();		// グラフィックの描画
void DrawStfInfo();		// スタッフの描画

void DrawFadeInfo();	// フェードＩＯ情報の反映

void EndingSCLDecode();	// エンディング用 SCL のデコード


void EndingSetFixedColors(PALETTE& pal)
{
	pal[255] = { 0x00, 0x00, 0x00 };
	pal[199] = { 0xFF, 0xFF, 0xFF };
	pal[198] = { 0x80, 0x80, 0x80 };
}



// エンディングまわりの初期化 //
bool EndingInit(void)
{
	PALETTE	pal;

	GrpBackend_SetClip(GRP_RES_RECT);
	GrpBackend_Clear();
	Grp_Flip();
	GrpBackend_Clear();

	if(!LoadGraph(GRAPH_ID_ENDING) || !LoadStageData(GRAPH_ID_ENDING)) {
		return false;
	}
	BGM_Stop();


	GrpBackend_PaletteGet(pal);
	EndingSetFixedColors(pal);
	GrpBackend_PaletteSet(pal);


	GameMain = EndingProc;

	FlashState = 0;

	EGrpInfo.bWantDisp = false;
	EStfTask.bWantDisp = false;

	TextObj.Clear();
	EText.NumText = 0;
	EText.Rect = TextObj.Register({ GRP_RES.w, 131 });
	EText.Rerender = true;

	return true;
}


void EndingProc(bool&)
{/*
	if(Key_Data){
		GameExit();
		return;
	}
*/
	extern bool IsDraw();

	if(FlashState) FlashState -= 32;

	EndingSCLDecode();
	if(GameMain != EndingProc) return;

	if(IsDraw()){
		UpdateGrpInfo();
		UpdateStfInfo();
		EndingDraw();
	}
}


// エンディング時の描画処理 //
void EndingDraw(void)
{
	// 画面消去 //
	GrpBackend_Clear(255, RGB{ 0, 0, 0 });

	// それぞれのグラフィックを描画するで //
	DrawGrpInfo();
	DrawStfInfo();
	if(EText.Rerender) {
		EText.Prerender();
		EText.Rerender = false;
	}
	TextObj.Blit({ 0, 349 }, EText.Rect);

	// フェード情報の反映ぢゃ //
	DrawFadeInfo();

	Grp_Flip();
}


// グラフィックのフェードアウト用関数 //
void FadeoutPaletteGrp(PALETTE& Dest, const PALETTE& Src, uint8_t a)
{
	Dest = Src.Fade(a, 0, 199);
	EndingSetFixedColors(Dest);
}


// スタッフ名のフェードアウト用関数 //
void FadeoutPaletteStf(PALETTE& Dest, const PALETTE &Src, uint8_t a)
{
	Dest = Src.Fade(a, 200, 255);
	EndingSetFixedColors(Dest);
}


// グラフィックの更新(内部データ) //
void UpdateGrpInfo()
{
	EGrpInfo.timer++;
	if(EGrpInfo.timer > EGrpInfo.fadeout){
		if(EGrpInfo.alpha-3 > 0) EGrpInfo.alpha-= 3;
		else                     EGrpInfo.alpha = 0;
	}
	else if(EGrpInfo.timer > EGrpInfo.fadein){
		if(EGrpInfo.alpha+3 < 255) EGrpInfo.alpha+= 3;
		else                       EGrpInfo.alpha = 255;
	}

	if(EGrpInfo.bWantDisp && EGrpInfo.alpha==0)
		EGrpInfo.bWantDisp = false;
}


// スタッフの更新(内部データ)
void UpdateStfInfo()
{
	EStfTask.timer++;
	if(EStfTask.timer > EStfTask.fadeout){
		if(EStfTask.alpha-3 > 0) EStfTask.alpha-= 3;
		else                     EStfTask.alpha = 0;
	}
	else if(EStfTask.timer > EStfTask.fadein){
		if(EStfTask.alpha+3 < 255) EStfTask.alpha+= 3;
		else                       EStfTask.alpha = 255;
	}

	if(EStfTask.bWantDisp && EStfTask.alpha==0)
		EStfTask.bWantDisp = false;
}


// グラフィックの描画 //
void DrawGrpInfo()
{
	if(!EGrpInfo.bWantDisp) return;

	// 驚異の画像表示 //
	const auto sid = (SURFACE_ID::ENDING_PIC + (EGrpInfo.target - EndingGrp));
	GrpSurface_BlitOpaque({ EGrpInfo.x, EGrpInfo.y }, sid, { 0, 0, 320, 240 });
}


// スタッフの描画 //
void DrawStfInfo()
{
	if(!EStfTask.bWantDisp) return;

	auto Blit = [](WINDOW_POINT dst, const PIXEL_LTRB& src) {
		dst -= (src.Size() / 2);
		GrpSurface_Blit({ dst.x, dst.y }, SURFACE_ID::ENDING_CREDITS, src);
	};

	Blit({ EStfTask.ox, EStfTask.oy }, StaffLabel[EStfTask.TitleID]);
	for(decltype(EStfTask.NumStf) i = 0; i < EStfTask.NumStf; i++){
		const WINDOW_POINT dst = { EStfTask.ox, (EStfTask.oy + (i * 30) + 50) };
		Blit(dst, StaffMember[EStfTask.StfID[i]]);
	}
}


// テキストの描画 //
void EndingText::Prerender()
{
	TextObj.Prerender(Rect, [this](TEXTRENDER_SESSION auto& s) {
		int max = 0;

		for(decltype(NumText) i = 0; i < NumText; i++) {
			max = (std::max)(max, static_cast<int>(Text[i].size()));
		}

		const auto dx = (8 * (39 - (max / 2)));

		s.SetFont(FONT_ID::NORMAL);
		s.SetColor({ 128, 128, 128 });
		for(decltype(NumText) i = 0; i < NumText; i++) {
			s.Put({ (dx + 21), (1 + (i * 25)) }, Text[i]);
			s.Put({ (dx + 19), (1 + (i * 25)) }, Text[i]);
			s.Put({ (dx + 20), (0 + (i * 25)) }, Text[i]);
			s.Put({ (dx + 20), (2 + (i * 25)) }, Text[i]);
		}

		s.SetColor({ 255, 255, 255 });
		for(decltype(NumText) i = 0; i < NumText; i++) {
			s.Put({ (dx + 20), (1 + (i * 25)) }, Text[i]);
		}
	});
}


void FlashPaletteGrp(PALETTE& dest, const PALETTE& pal, uint16_t a)
{
	const uint16_t a16 = ((a > 256) ? (a - 256): a);
	for(int i = 0; i < dest.size(); i++){
		dest[i].r = (std::min)(256, (256 * (256 - a) + (pal[i].r * a16)) / 256);
		dest[i].g = (std::min)(256, (256 * (256 - a) + (pal[i].g * a16)) / 256);
		dest[i].b = (std::min)(256, (256 * (256 - a) + (pal[i].b * a16)) / 256);
	}
}


// フェードＩＯ情報の反映 //
void DrawFadeInfo()
{
	PALETTE	temp_pal;

	// フェードアウト関連
	if(GrpGeom_FB()) {
		if(FlashState){
			FlashPaletteGrp(temp_pal, EGrpInfo.target->pal, FlashState);
			GrpBackend_PaletteSet(temp_pal);
		}
		else if(EGrpInfo.target){
			FadeoutPaletteGrp(
				temp_pal,
				EGrpInfo.target->pal,
				Cast::down_sign<uint8_t>(EGrpInfo.alpha)
			);
			FadeoutPaletteStf(
				temp_pal, temp_pal, Cast::down_sign<uint8_t>(EStfTask.alpha)
			);
			GrpBackend_PaletteSet(temp_pal);
		}
		else{
			temp_pal = { 0 };
			EndingSetFixedColors(temp_pal);
			GrpBackend_PaletteSet(temp_pal);
		}
	}
	else{
		GrpGeom->Lock();

		if(EGrpInfo.bWantDisp){
			GrpGeom->SetAlphaNorm(255 - EGrpInfo.alpha);
			GrpGeom->SetColor({ 0, 0, 0 });
			GrpGeom->DrawBoxA(
				EGrpInfo.x, EGrpInfo.y, (EGrpInfo.x + 320), (EGrpInfo.y + 240)
			);
		}
		if(EStfTask.bWantDisp){
			GrpGeom->SetAlphaNorm(255 - EStfTask.alpha);
			GrpGeom->SetColor({ 0, 0, 0 });
			if(EStfTask.ox == 320) {
				GrpGeom->DrawBoxA(0, 0, GRP_RES.w, GRP_RES.h);
			} else if(EStfTask.ox > 320) {
				GrpGeom->DrawBoxA(320, 0, GRP_RES.w, 300);
			} else {
				GrpGeom->DrawBoxA(0, 0, (320 - 50), 300);
			}
		}
		if(FlashState){
			GrpGeom->SetAlphaNorm(255 - FlashState);
			GrpGeom->SetColor({ 5, 5, 5 });
			GrpGeom->DrawBoxA(0, 0, GRP_RES.w, GRP_RES.h);
		}

		GrpGeom->Unlock();
	}
}



// エンディング用 SCL のデコード //
void EndingSCLDecode()
{
	bool bFlag = true;

	while(bFlag){
		const uint8_t *cmd = SCL_Now;
		switch(cmd[0]){
			case(SCL_TIME): {
				const auto temp = I32LEAt(&cmd[1]);
				if(temp > GameCount) {
					bFlag = false;
				} else {
					SCL_Now += 5; // cmd(1)+time(4)
				}
				break;
			}

			case(SCL_MSG):		// メッセージを出力する
				EText.Text[EText.NumText++] = std::bit_cast<const char *>(cmd + 1);
				EText.Rerender = true;
				SCL_Now += (strlen(std::bit_cast<const char *>(cmd + 1)) + 2);
			break;

			case(SCL_FACE):		// 顔を表示する
				switch(cmd[1]){
				case 0:
					EGrpInfo.fadein  = 0;
					EGrpInfo.fadeout = 128+64+64+512;
					EGrpInfo.x       = 640-40-320;
					EGrpInfo.y       = 40;
				break;

				case 1:case 2: case 3:
					EGrpInfo.fadein  = 0;
					EGrpInfo.fadeout = 128+64;
					EGrpInfo.x       = 320-160;
					EGrpInfo.y       = 40;
				break;

				case 5:
					EGrpInfo.fadein  = 0;
					EGrpInfo.fadeout = 128+64+64+(512+512)*2;
					EGrpInfo.x       = 40;
					EGrpInfo.y       = 40;
				break;

				default:
					EGrpInfo.fadein  = 0;
					EGrpInfo.fadeout = 128+64+64+512;
					EGrpInfo.x       = 40;
					EGrpInfo.y       = 40;
				break;
				}
				EGrpInfo.alpha   = 0;
				EGrpInfo.target  = EndingGrp + cmd[1];
				EGrpInfo.timer   = 0;
				EGrpInfo.bWantDisp = true;
				SCL_Now += 2;
			break;

			case(SCL_STAFF):	// わかりにくいが、１２８を加えると、役割名指定ね
				if(cmd[1] >= 128){
					switch(cmd[1] - 128){
						case 0:case 4:
							EStfTask.fadein   = 0;
							EStfTask.fadeout  = 128+64+64+128;
							EStfTask.ox       = 320+130;
							EStfTask.oy       = 80+50;
						break;
						case 2:case 5:
							EStfTask.fadein   = 0;
							EStfTask.fadeout  = 128+64+64+128;
							EStfTask.ox       = 320+130;
							EStfTask.oy       = 80;
						break;
						case 1:case 3:
							EStfTask.fadein   = 0;
							EStfTask.fadeout  = 128+64+64+128;
							EStfTask.ox       = 130;
							EStfTask.oy       = 80+50;
						break;
						case 6:
							EStfTask.fadein   = 0;
							EStfTask.fadeout  = 128+64+64;//+64;
							EStfTask.ox       = 320;
							EStfTask.oy       = 80+80;
						break;
					}
					EStfTask.alpha    = 0;
					EStfTask.timer    = 0;
					EStfTask.timer    = 0;
					EStfTask.NumStf   = 0;
					EStfTask.TitleID  = cmd[1]-128;
					EStfTask.bWantDisp = true;
				}
				else{
					EStfTask.StfID[EStfTask.NumStf++] = cmd[1];
				}
				SCL_Now+=2;
			break;

			case(SCL_NPG):		// 新しいページに変更する
				EText.NumText = 0;
				EText.Rerender = true;
				SCL_Now++;
			break;

			case(SCL_END):		// カウントも変更させずにリターンするのだ
				EGrpInfo.bWantDisp = false;
				EStfTask.bWantDisp = false;
				NameRegistInit(false);
			return;

			case(SCL_MUSIC):
				BGM_Switch(cmd[1]);
				SCL_Now += 2;
			break;

			case(SCL_EFC):
				switch(cmd[1]){
					case 0:		FlashState = 256*2;		break;
				}

				SCL_Now += 2;
			break;

			case(SCL_STAGECLEAR):	// ステージクリア
				// ステージクリア処理をここに記述 //
				//GameNextStage();	// 本当はエラーチェックが必要!!
			return;

			case(SCL_GAMECLEAR):
				//if(GameStage == 6) GameStage = 7;
				//NameRegistInit();
			return;

			default:	// 未実装 or ばぐ
			return;
		}
	}

	GameCount++;
}
