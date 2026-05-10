/*
 *   String effects
 *
 *   (Game logic because [SEFC_STR1_2] uses the logic's RNG)
 */

#include "ssg/internal/SEffect.hpp"
#include "GIAN.H"
#include "ssg/internal/RNG.hpp"
#include "hatoyama/logic/ut_math.h"


C_SEFFECT SEffect;



// エフェクトの初期化を行う //
void C_SEFFECT::Init(void)
{
	for(auto& it : SEffect) {
		//memset(SEffect+i,0,sizeof(SEFFECT_DATA));
		it.cmd = SEFC_NONE;
	}
}

// 文字列系エフェクト //
void C_SEFFECT::SetString(int x, int y, const char *s)
{
	int		i,j,len;

	len = strlen(s);
	for(i=j=0;i<len;i++){
		while(SEffect[j].cmd!=SEFC_NONE){
			j++;
			if(j>=SEFFECT_MAX) return;
		}
		SEffect[j].c    = s[i];
		SEffect[j].x    = (x + (i<<4) + 512)<<6;
		SEffect[j].y    = (y)<<6;
		SEffect[j].vx   = (-20)<<6;
		SEffect[j].vy   = (0)<<6;
		SEffect[j].cmd  = SEFC_STR1;
		SEffect[j].time = 26;
	}
}

void C_SEFFECT::SetPoint(WORLD_COORD x, WORLD_COORD y, int32_t point)
{
	for(auto& it : SEffect) {
		if(it.cmd != SEFC_NONE) {
			continue;
		}

		it.point = point;
		it.x     = x;
		it.y     = y;
		it.vx    = 0;
		it.vy    = ((-64 * 3) + 32);
		it.cmd   = SEFC_STR2;
		it.time  = 90;
		return;
	}
}

// ゲームオーバーの表示 //
void C_SEFFECT::SetGameOver(void)
{
	for(auto& it : SEffect) {
		if(it.cmd != SEFC_NONE) {
			continue;
		}

		it.x     = GX_MID;
		it.y     = (GY_MID - (64 * (60 + 40)));
		it.vx    = 0;
		it.vy    = 0;
		it.cmd   = SEFC_GAMEOVER;
		it.time  = 120-35;//100;
		return;
	}
}

// 曲名の表示 //
void C_SEFFECT::SetMTitle(WINDOW_COORD y, const PIXEL_SIZE& extent)
{
	// 空きバッファ検索 //
	auto e = std::ranges::find_if(SEffect, [](const auto& e) {
		return (e.cmd == SEFC_NONE);
	});
	if(e == std::end(SEffect)) {
		return;
	}

	// 曲のタイトルが長すぎる場合、どうしましょう？ //
	auto x = (std::max)((640 - 128 - 32 - extent.w), 128);

	e->cmd  = SEFC_MTITLE1;
	e->x    = (x << 6); // + ((64 * 2) * 16);
	e->y    = (y << 6);
	e->time = (64 * 2);
	e->vx   = extent.w;
	e->vy   = extent.h;
}

// エフェクトを動かす(仕様変更の可能性があります) //
void C_SEFFECT::Move(void)
{
	for(auto& it : SEffect) {
		auto *e = &it;
		switch(e->cmd){
			case(SEFC_STR1):
				e->x += e->vx;
				e->y += e->vy;
				if(e->time==0){
					e->cmd  = SEFC_STR1_2;
					e->time = 256;//128;
				}
			break;

			case(SEFC_STR1_3):
				e->x += e->vx;
				e->y += (e->vy+=16);
				if(e->time==0) e->cmd = SEFC_NONE;
			break;

			case(SEFC_STR1_2):
				if(e->time==0){
					const uint8_t deg = (128 + (RNG.next() % 128));
					e->cmd  = SEFC_STR1_3;
					e->time = 64;
					e->vx   = cosl(deg,10*64);
					e->vy   = sinl(deg,10*64);
				}
			break;

			case(SEFC_STR2):
				if(e->time==0) e->cmd = SEFC_NONE;
				e->x += e->vx;
				e->y += (e->vy+=3);
			break;

			case(SEFC_GAMEOVER):
				if(e->time==0){
					e->cmd  = SEFC_GAMEOVER2;
					e->time = 35;
				}
			break;

			case(SEFC_MTITLE1):		// 曲名出現
				//e->x -= 16;
				if(e->time==0){
					e->cmd  = SEFC_MTITLE2;
					e->time = 64*4;
				}
			break;

			case(SEFC_MTITLE2):		// 曲名停止
				if(e->time==0){
					e->cmd  = SEFC_MTITLE3;
					e->time = 64*2;
				}
			break;

			case(SEFC_MTITLE3):		// 曲名消去
				e->x += 64;
				if(e->time==0){
					e->cmd = SEFC_NONE;
				}
			break;

			case(SEFC_NONE):default:
			break;
		}
		e->time--;
	}
}
