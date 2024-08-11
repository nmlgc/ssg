/*                                                                           */
/*   GIAN.cpp   ゲーム全体の管理                                             */
/*                                                                           */
/*                                                                           */

#include "GIAN.H"
#include "FONTUTY.H"
#include "LEVEL.H"
#include "CONFIG.H"
#include "platform/time.h"


///// [グローバル変数] /////
//HIGH_SCORE		*HighScore;
//char			ScoreTable[8][80];
uint32_t	GameCount;
uint8_t	GameStage;
uint8_t	GameLevel;



///// [ 関数(非公開) ] /////
extern void StdStatusOutput(void)
{
	static uint32_t prev;
	static uint32_t fps, count;
	//extern InputConfig			IConfig;
	const char *const DItem[4] = { "Easy", "Norm", "Hard", "Luna" };
	char	buf[100];

#ifdef PBG_DEBUG
	if(!DebugDat.MsgDisplay) return;

	const auto now = Time_SteadyTicksMS();
	if((now - prev) <= 1000) {
		count++;
	} else {
		fps   = count;
		count = 0;
		prev  = now;
	}

	sprintf(buf, "%03u Fps", fps);
	GrpPut16(0,0,buf);

	sprintf(buf, "%2dBppMode", ConfigDat.BitDepth.v.value());
	GrpPut16(0,32,buf);
	//sprintf(buf,"%s",DItem[ConfigDat.GameLevel.v]);
	//GrpPut16(0,50,buf);

	sprintf(buf,"%s",DItem[PlayRank.GameLevel]);
	GrpPut16(0,50,buf);
	sprintf(buf,"%s<Lvl>",DItem[GameLevel]);
	GrpPut16(0,68,buf);
	sprintf(buf,"Pr %d",PlayRank.Rank);
	GrpPut16(0,86,buf);


	sprintf(buf,"Enemy %3d",EnemyNow);
	GrpPut16(0,96+40,buf);

	sprintf(buf,"Tama1 %3d",Tama1Now);
	GrpPut16(0,128+40,buf);
	sprintf(buf,"Tama2 %3d",Tama2Now);
	GrpPut16(0,148+40,buf);
	sprintf(buf,"Laser %3d",LaserNow);
	GrpPut16(0,176+40,buf);
	sprintf(buf,"HLaser %2d",HLaserNow);
	GrpPut16(0,196+40,buf);

	sprintf(buf,"MTama %3d",MaidTamaNow);
	GrpPut16(0,224+40,buf);

	sprintf(buf,"Item  %3d",ItemNow);
	GrpPut16(0,252+40,buf);

	sprintf(buf,"Pow   %3d",Viv.exp);
	GrpPut16(0,290+40,buf);

	sprintf(buf,"SSPD  %3d",ScrollInfo.ScrollSpeed);
	GrpPut16(0,320+40,buf);

	GrpPut16(0,440,"Gian07");
	GrpPut16(0,460,"DebugMode");

	GrpPut16(640-128,100,"SCL Count");
	sprintf(buf," %5d",GameCount);
	GrpPut16(640-128,120,buf);
#else
//	GrpPut16(0,440,"G07");
//	GrpPut16(0,460,"12/5 Ver");
#endif

	const auto tm = Time_NowLocal();

	sprintf(buf, "%02u/%02u/%02u", tm.month, tm.day, (tm.year % 100u));
	GrpPut16(640-128,0,"Date");
	GrpPut16(640-128,20,buf);

	sprintf(buf, "%02u:%02u:%02u", tm.hour, tm.minute, tm.second);
	GrpPut16(640-128,50,"Time");
	GrpPut16(640-128,70,buf);

#ifndef PBG_DEBUG // pbg quirk
	sprintf(buf,"Bomb   %d",Viv.bomb);
	GrpPut16(640-128,400,buf);
#endif

	sprintf(buf,"Left   %d",Viv.left);
	GrpPut16(640-128,440,buf);
	sprintf(buf,"Credit %d",Viv.credit);		// -1 に注意だ！！
	GrpPut16(640-128,460,buf);
}
