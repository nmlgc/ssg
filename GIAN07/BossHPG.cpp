/*
 *   Boss HP gauge
 *
 */

#include "BossHPG.h"
#include "GEOMETRY.H"
#include "hatoyama/logic/ut_math.h"


///// [ 定数 ] /////

// 体力ゲージ編 //
constexpr WINDOW_COORD BOSSHPG_WIDTH = 256;	// 体力ゲージの幅
constexpr WINDOW_COORD BOSSHPG_HEIGHT = 24;	// 体力ゲージの高さ
constexpr WINDOW_COORD BOSSHPG_START_X = X_MAX;	// 体力ゲージの初期Ｘ
constexpr WINDOW_COORD BOSSHPG_END_X = 260;	// 体力ゲージの最終Ｘ

#define BHPG_DEAD	0x00	// 体力ゲージは使用されていない
#define BHPG_OPEN1	0x01	// 体力ゲージを開く(第一エフェクト中)
#define BHPG_OPEN2	0x02	// 体力ゲージを開く(体力上昇中)
#define BHPG_NORM	0x03	// 体力ゲージの準備ができている
#define BHPG_CLOSE	0x04	// 体力ゲージを閉じる
#define BHPG_OPEN3	0x05	// 体力ゲージを更新する



///// [構造体] /////

// ボスの体力ゲージ //
typedef struct tagBOSSHPG_INFO{
	uint32_t	Now, Max;	// 体力の現在値＆最大値
	uint32_t	Next;	// 次の体力の値
	uint32_t	Update;	// 更新用の値
	uint32_t	Count;	// フレーム数保持

	uint16_t	XTemp[BOSSHPG_HEIGHT];	// ＨＰゲージの演出用
	uint8_t	State;	// 状態
} BOSSHPG_INFO;



///// [ 変数 ] /////
BOSSHPG_INFO	BossHPG;			// 体力ゲージ保持用



void BossHPG_Init(void)
{
	BossHPG.State = BHPG_DEAD;
}

// ボスの体力ゲージをオープンする //
void BossHPG_Open(uint32_t max)
{
	int		i;

	BossHPG.Max    = max;		// 最大値
	BossHPG.Now    = 0;			// 最初のエフェクトで上昇して行くので
	BossHPG.Next   = max;		// 次の体力値
	BossHPG.Update = max;		// 更新用の値

	BossHPG.State = BHPG_OPEN1;
	BossHPG.Count = 0;

	// 表示用初期Ｘを指定する(乱数を使用するが...) //
	for(i=0;i<BOSSHPG_HEIGHT;i++){
		BossHPG.XTemp[i] = BOSSHPG_START_X+i*20;
	}
}


// ボスの体力ゲージを上昇させる //
void BossHPG_Update(uint32_t next)
{
//	BossHPG.Max  = max;		// 最大値
//	BossHPG.Now  = 0;		// 最初のエフェクトで上昇して行くので
	BossHPG.Update = next;	// 次の体力値

	BossHPG.State = BHPG_OPEN3;
//	BossHPG.Count = 0;
}


// ボスの体力ゲージを増減する //
void BossHPG_Move(uint32_t now)
{
	int		i;
	int		ChkCount = 0;

	BossHPG.Next = now;

	switch(BossHPG.State){
		case(BHPG_OPEN1): {
			for(auto& it : BossHPG.XTemp) {
				it -= 6;
				if(it <= BOSSHPG_END_X){
					it = BOSSHPG_END_X;
					ChkCount++;
				}
			}

			if(ChkCount==BOSSHPG_HEIGHT) BossHPG.State = BHPG_OPEN2;
		}
		break;

		case(BHPG_OPEN2):
			BossHPG.Now += ((BossHPG.Max>>7)+1);
			if(BossHPG.Now>=BossHPG.Max){
				BossHPG.Now   = BossHPG.Max;
				BossHPG.State = BHPG_NORM;
			}
		break;

		case(BHPG_OPEN3):
			BossHPG.Now += ((BossHPG.Max>>7)+1);
			if(BossHPG.Now >= BossHPG.Update){
				BossHPG.Now   = BossHPG.Update;
				BossHPG.State = BHPG_NORM;
			}
		break;

		case(BHPG_NORM):
			if(BossHPG.Now > BossHPG.Next){
				//temp = max(BossHPG.Max>>10,1);
				//temp = max((30*8*3)/max(BossHPG.Max,1),3);
				const auto temp = (std::max)(
					((std::max)(BossHPG.Max, 1u) / (30 * 8 * 4)), 3u
				);
				if(BossHPG.Now-BossHPG.Next>temp) BossHPG.Now -= temp;
				else                              BossHPG.Now = BossHPG.Next;
			}
			if(BossHPG.Now==0) BossHPG_Close();
		break;

		case(BHPG_CLOSE):
			BossHPG.XTemp[BOSSHPG_HEIGHT-1] += 6;
			for(i=BOSSHPG_HEIGHT-2;i>=0;i--){
				BossHPG.XTemp[i] = max(BossHPG.XTemp[i],BossHPG.XTemp[i+1]-20);
			}
			if(BossHPG.XTemp[0]>=BOSSHPG_START_X)
				BossHPG_Close();
		break;

		case(BHPG_DEAD):
			// もちろん何もしない //
		return;
	}

	BossHPG.Count++;
}

// ボスの体力ゲージをクローズする //
void BossHPG_Close(void)
{
	// 後で変更のこと //
	BossHPG.State = BHPG_CLOSE;
}

// ボスの体力ゲージを描画する //
void BossHPG_Draw(void)
{
	PIXEL_LTRB	src;
	int i;

	switch(BossHPG.State){
		case(BHPG_OPEN1):case(BHPG_CLOSE):
			// エフェクト付き枠の描画 //
			for(i=0;i<BOSSHPG_HEIGHT;i++){
				src = { 0, (104 + i), BOSSHPG_WIDTH, (104 + i + 1) };
				GrpSurface_Blit(
					{ BossHPG.XTemp[i], (16 + i) }, SURFACE_ID::SYSTEM, src
				);
			}
		break;

		case(BHPG_OPEN2): case(BHPG_NORM): case(BHPG_OPEN3): {
			// 体力ゲージの描画 //
			constexpr WINDOW_COORD left   = (BOSSHPG_END_X + 3);
			constexpr WINDOW_COORD top    = (16 + 3);
			constexpr WINDOW_COORD bottom = (top + 11);
			const auto x1 = (left + ((BossHPG.Next * 30 * 8) / BossHPG.Max));
			const auto x2 = (left + ((BossHPG.Now  * 30 * 8) / BossHPG.Max));
			constexpr uint8_t alpha = (128 + 64);
			constexpr RGB216 col    = { 0, 1, 5 };

			GrpGeom->Lock();
			GrpGeom->SetAlphaNorm(alpha);
			if(auto *gp = GrpGeom_Poly()) {
				VERTEX_XY Src[4] = {
					{ 0, top },
					{ left, top },
					{ left, bottom },
					{ 0, bottom },
				};
				if(x1<x2){
					Src[0].x = Src[3].x = x1;
					GeomGrdRectA(*gp, Src, col.ToRGB().WithAlpha(alpha));
					// gp->DrawBoxA(left, top, x1, bottom);
					gp->SetColor({ 5, 0, 0 });
					gp->DrawBoxA(x1, top, x2, bottom);
				}
				else{
					Src[0].x = Src[3].x = x2;
					GeomGrdRectA(*gp, Src, col.ToRGB().WithAlpha(alpha));
					// gp->DrawBoxA(left, top, x2, bottom);
				}
			} else if(auto *gf = GrpGeom_FB()) {
				constexpr auto line_top = (top + 5);
				constexpr auto line_bottom = (bottom - 4);
				gf->SetColor(col);
				if(x1<x2){
					gf->DrawBoxA(left, top, x2, line_top);
					gf->DrawBoxA(left, line_bottom, x2, bottom);
					gf->SetColor({ 5, 5, 5 });
					gf->DrawBoxA(left, line_top, x1, line_bottom);
					gf->SetColor({ 5, 0, 0 });
					gf->DrawBoxA(x1, top, x2, bottom);
				}
				else{
					gf->DrawBoxA(left, top, x2, line_top);
					gf->DrawBoxA(left, line_bottom, x2, bottom);
					gf->SetColor({ 5, 5, 5 });
					gf->DrawBoxA(left, line_top, x2, line_bottom);
				}
			}

			GrpGeom->Unlock();

			// 枠の描画 //
			src = { 0, 104, BOSSHPG_WIDTH, 128 };
			GrpSurface_Blit({ BOSSHPG_END_X, 16 }, SURFACE_ID::SYSTEM, src);
		}
		break;

		case(BHPG_DEAD):
			// もちろん何もしない //
		break;
	}
}
