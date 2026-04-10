/*
 *   Rendering and management code for the in-game HUD
 *
 */

// GCC 15 throws `error: conflicting declaration 'typedef struct imaxdiv_t
// imaxdiv_t'` if this appears after a module import.
#include <inttypes.h> // for PRId64
#include <printf/printf.h>

#include "HUD.H"
#include "EFFECT.H"
#include "FONTUTY.H"
#include "ITEM.H"
#include "LogicInstance.h"
#include "MAID.H"
#include "MAIDTAMA.H"
#ifdef PBG_DEBUG
#include "CONFIG.H"
#include "ENEMY.H"
#include "HOMINGL.H"
#include "PRankCtrl.h"
#include "SCL.H"
#include "SCROLL.H"
#include "ssg/internal/Stage.hpp"
#endif
#include "hatoyama/engine/text_backend.h"
#include "hatoyama/engine/time.h"
#include "hatoyama/logic/ut_math.h"

/// BGM title
/// ---------

TEXTRENDER_RECT_ID MTitleRect;

// Print the ♪ separately, since it can be combined with a music title in
// any encoding.
Narrow::string_view MTitleStrs[2] = { "♪ " };

void MTitleInit(void)
{
	MTitleRect = TextObj.Register({ X_SIZ, 20 });
}

void MTitleSet(WINDOW_COORD y, Narrow::string_view s)
{
	MTitleStrs[1] = s;
	PIXEL_SIZE extent = { 0, 0 };
	for(const auto s : MTitleStrs) {
		const auto s_extent = TextObj.TextExtent(FONT_ID::NORMAL, s);
		extent.w += s_extent.w;
		extent.h = s_extent.h;
	};
	SEffectSetMTitle(y, extent);
}

void MTitleRender(WINDOW_POINT topleft, const PIXEL_LTWH& subrect)
{
	const auto mtitle = MTitleStrs[1];
	TextObj.Render(topleft, MTitleRect, mtitle, [](TEXTRENDER_SESSION& s) {
		const auto gradient_func = [](PIXEL_COORD y) -> uint8_t {
			return (255 + 8 - (y * 8));
		};
		DrawGrdFont(s, MTitleStrs, FONT_ID::NORMAL, true, gradient_func);
	}, subrect);
}
/// ---------

// エフェクトを描画する(仕様変更の可能性があります) //
void SEffectDraw(void)
{
	int j, k;
	int				x,y;
	int				temp;
	PIXEL_LTWH src;
	char			buf[20];

	for(const auto& it : SEffect) {
		const auto *e = &it;
		switch(e->cmd){
			case(SEFC_STR1):case(SEFC_STR1_2):case(SEFC_STR1_3):
				GrpPutc(e->x>>6,e->y>>6,e->c);
			break;

			case(SEFC_STR2):
				sprintf(buf, "%u", e->point);
				GrpPutScore(e->x>>6,e->y>>6,buf);
			break;

			case(SEFC_GAMEOVER):
				strcpy(buf,"GAME OVER");
				for(j=0;j<9;j++){
					// *37 //
					const auto angle = Cast::down<uint8_t>(
						(e->time * 3) + (j * 26)
					);
					x = (e->x >> 6) + cosl(angle, e->time * 4);
					y = (e->y >> 6) + sinl(angle, e->time * 4);
					GrpPutc(x,y,buf[j]);
				}
			break;

			case(SEFC_GAMEOVER2):
				x = (e->x >> 6) + 8;
				y = (e->y >> 6) + 8;
				j = (35 - e->time) / 2;
				GrpGeom->Lock();
				GrpGeom->SetColor({ 0, 0, 0 });
				GrpGeom->SetAlphaNorm(Cast::down<uint8_t>((35 - e->time) * 3));
				GrpGeom->DrawBoxA((x - 170), (y - j), (x + 170), (y + j));
				GrpGeom->Unlock();

				strcpy(buf,"GAME OVER");
				for(j=0;j<9;j++){
					// *37 //
					x = (e->x>>6) + (j-4)*(35-e->time);
					y = (e->y>>6);
					GrpPutc(x,y,buf[j]);
				}
			break;

			case(SEFC_MTITLE3): {
				const auto degx = Cast::down<uint8_t>((64 * 2) - e->time);
				for(j=0;j<e->vx;j++){
					src = { j, 0, 1, e->vy };
					temp = sinl(degx,100);
					const auto y = (
						(e->y >> 6) - sinl((degx + j), degx /*40*/)
					);
					for(k=0;k<2;k++){
						const auto x = (
							(e->x >> 6) + sinl((degx + (j / 2)), temp) + j + k
						);
						MTitleRender({ x, y }, src);
					}
				}
			}
			break;

			case(SEFC_MTITLE1): {
				const auto degx = Cast::down<uint8_t>(e->time);
				for(j=0;j<e->vx;j++){
					src = { j, 0, 1, e->vy };
					temp = sinl(degx,160);
					const auto y = (
						(e->y >> 6) - sinl((degx + j), degx /*40*/)
					);
					for(k=0;k<2;k++){
						const auto x = (
							(e->x >> 6) + sinl((degx + (j / 2)), temp) + j + k
						);
						MTitleRender({ x, y }, src);
					}
				}
			}
			break;

			case(SEFC_MTITLE2): {
				GrpGeom->Lock();
				GrpGeom->SetColor({ 0, 0, 0 });
				GrpGeom->SetAlphaNorm(
					(sinl(Cast::down<uint8_t>(e->time - 32), 80) + 80)
				);
				for(j=0;j<16;j++){
					temp = sinl(128+j*16,16);
					GrpGeom->DrawBoxA(
						((e->x >> 6) + temp - 16),
						((e->y >> 6) + j),
						((X_MAX - 16) - temp),
						((e->y >> 6) + j + 1)
					);
				}
				GrpGeom->Unlock();
				src = { 0, 0, e->vx, e->vy };
				MTitleRender({ (e->x >> 6), (e->y >> 6) }, src);
			}
			break;

			case(SEFC_NONE):default:
			break;
		}
	}
}

// 各種ステータスを描画する //
void StateDraw(void)
{
	int		i,temp;
	//PIXEL_LTRB	src = { 0, 80, 128, (80 + 16) };
	static const PIXEL_LTRB src = { 0, 80, 128, (80 + 24) };
	char	buf[100];

	// 残りかすり時間ゲージの表示 //
	if(Viv.evade_c){
		GrpGeom->Lock();
		GrpGeom->SetColor({ 5, 1, 0 });
		GrpGeom->SetAlphaOne();
		for(i=0;i<=10;i++){
			temp = 128+9+(Viv.evade_c>>2) + (5-i);
			if(temp > (128 + 8)) {
				GrpGeom->DrawBoxA(
					(128 + 8), (16 + 3 + i), temp, (16 + 3 + i + 1)
				);
				// GrpGeom->DrawBoxA(
				// 	(128 + 8),
				// 	(16 + 3),
				// 	(128 + 9 + (Viv.evade_c >> 2)),
				// 	(16 + 3 + 10)
				// );
			}
		}
		GrpGeom->Unlock();
	}
	GrpSurface_Blit({ 128, 16 }, SURFACE_ID::SYSTEM, src);
	sprintf(buf,"%3d",Viv.evade);
	GrpPut57(128 + 95,16 + 91-80,buf);

	// 得点表示 //
	sprintf(buf, ("%9" PRId64), Viv.score);
	GrpPut16(128, 0, buf);

	// 仮の残機＆残りボム表示 //
	sprintf(buf,"       Bomb %1d", Viv.bomb);
	GrpPut16(280, 0, buf);

	// 残りメイド数表示 //
	for(i=0; i<Viv.left; i++){
		constexpr PIXEL_LTWH src = { 608, 432, 16, 16 };
		GrpSurface_Blit({ (280 + (i * 14)), 0 }, SURFACE_ID::SYSTEM, src);
	}
}

void StdStatusOutput(void)
{
	const WINDOW_COORD column2_left = (GRP_RES.w - 128);

	static uint32_t prev;
	static uint32_t fps, count;
	//extern InputConfig			IConfig;
	const char *const DItem[4] = { "Easy", "Norm", "Hard", "Luna" };
	char buf[100];

#ifdef PBG_DEBUG
	if(!DebugDat.MsgDisplay) {
		return;
	}
#endif

	const auto now = Time_SteadyTicksMS();
	if((now - prev) <= 1000) {
		count++;
	} else {
		fps = count;
		count = 0;
		prev = now;
	}

	sprintf(buf, "%03u FPS", fps);
	GrpPut16(0, 0, buf);

#ifdef PBG_DEBUG
#ifdef SUPPORT_GRP_BITDEPTH
	sprintf(buf, "%2dBppMode", ConfigDat.BitDepth.v.value());
	GrpPut16(0, 32, buf);
#endif
	//sprintf(buf,"%s",DItem[ConfigDat.GameLevel.v]);
	//GrpPut16(0,50,buf);

	sprintf(buf, "%s", DItem[PlayRank.LevelRanked]);
	GrpPut16(0, 50, buf);
	sprintf(buf, "%s<Lvl>", DItem[Round.LevelSelected]);
	GrpPut16(0, 68, buf);
	sprintf(buf, "Pr %d", PlayRank.Rank);
	GrpPut16(0, 86, buf);


	sprintf(buf, "Enemy %3d", EnemyNow);
	GrpPut16(0, (96 + 40), buf);

	sprintf(buf, "Tama1 %3d", Tama1Now);
	GrpPut16(0, (128 + 40), buf);
	sprintf(buf, "Tama2 %3d", Tama2Now);
	GrpPut16(0, (148 + 40), buf);
	sprintf(buf, "Laser %3d", LaserNow);
	GrpPut16(0, (176 + 40), buf);
	sprintf(buf, "HLaser %2d", HLaserNow);
	GrpPut16(0, (196 + 40), buf);

	sprintf(buf, "MTama %3d", MaidTamaNow);
	GrpPut16(0, (224 + 40), buf);

	sprintf(buf, "Item  %3d", ItemNow);
	GrpPut16(0, (252 + 40), buf);

	sprintf(buf, "Pow   %3d", Viv.exp);
	GrpPut16(0, (290 + 40), buf);

	sprintf(buf, "SSPD  %3d", ScrollInfo.ScrollSpeed);
	GrpPut16(0, (320 + 40), buf);

	GrpPut16(0, 440, "Gian07");
	GrpPut16(0, 460, "DebugMode");

	GrpPut16(column2_left, 100, "SCL Count");
	sprintf(buf, " %5d", GameCount);
	GrpPut16(column2_left, 120, buf);
#else
	// GrpPut16(0,440,"G07");
	// GrpPut16(0,460,"12/5 Ver");
#endif

	const auto tm = Time_NowLocal();

	sprintf(buf, "%02u/%02u/%02u", tm.month, tm.day, (tm.year % 100u));
	GrpPut16(column2_left, 0, "Date");
	GrpPut16(column2_left, 20, buf);

	sprintf(buf, "%02u:%02u:%02u", tm.hour, tm.minute, tm.second);
	GrpPut16(column2_left, 50, "Time");
	GrpPut16(column2_left, 70, buf);

#ifndef PBG_DEBUG // pbg quirk
	sprintf(buf, "Bomb   %d", Viv.bomb);
	GrpPut16(column2_left, 400, buf);
#endif

	sprintf(buf, "Left %3d", Viv.left);
	GrpPut16(column2_left, 440, buf);
	sprintf(buf, "Credit %d", Viv.credit); // -1 に注意だ！！
	GrpPut16(column2_left, 460, buf);
}
