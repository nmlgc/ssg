/*
 *   Rendering and management code for the in-game HUD
 *
 */

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
