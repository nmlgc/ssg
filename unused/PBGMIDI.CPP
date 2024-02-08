/*
 *   Unused MIDI code
 *
 */

#define PBGMIDI_SOURCE_COMPILE
#include "DirectXUTYs/PBGMIDI.CPP"
#include "unused/PBGMIDI.H"

struct MID_DEVICE_UNUSED : public MID_DEVICE {
	WORD		fnmode;				// MIDI再生に使用する関数
	WORD		plmode;				// 曲の最後に達したときどうするか
};
extern MID_DEVICE_UNUSED Mid_Dev_unused;

struct MID_DATA_UNUSED : public MID_DATA {
	DWORDLONG	fticks;
	DWORDLONG	nticks;
};
extern MID_DATA_UNUSED Mid_Data_unused;

BOOL Mid_Start_unused(WORD fnmode,WORD plmode)
{
	Mid_Dev_unused.fnmode  = fnmode;
	Mid_Dev_unused.plmode  = plmode;
	Mid_Start();
}

void Mid_Play_unused(void)
{
	Mid_Play();

	switch(Mid_Dev_unused.fnmode){
		case(MIDFN_CALLBACK):
			Mid_GMReset();
			timeGetDevCaps(&Mid_Time.caps,sizeof(TIMECAPS));
			timeBeginPeriod(Mid_Time.caps.wPeriodMin);
			Mid_Time.delay  = 10;
			Mid_Time.htimer = timeSetEvent(Mid_Time.delay,Mid_Time.caps.wPeriodMin,
												CBMid_TimeFunc,0,TIME_PERIODIC);
		break;

		case(MIDFN_MIDLOOP):
			Mid_GMReset();
		break;

		default:
		return;
	}
}

void Mid_Stop_unused(void)
{
	switch(Mid_Dev_unused.fnmode){
		case(MIDFN_CALLBACK):
			timeKillEvent(Mid_Time.htimer);
			timeEndPeriod(Mid_Time.caps.wPeriodMin);
		break;

		case(MIDFN_MIDLOOP):
		break;

		default:
		return;
	}

	Mid_Stop();
}

static BOOL Mid_Init_unused(void)
{
	Mid_Data_unused.fticks     = 0;
	Mid_Data_unused.nticks     = 0;
	return Mid_Init();
}

void Mid_Volume_unused(BYTE volume)
{
	// これより下は削った方が良いかも //
	//temp.w.d1 = temp.w.d2 = volume;
	//midiOutSetVolume(Mid_Dev.mp,temp.dd);
}

void Mid_Pan(char pan)
{
	int		i;
	int		value = 0x40 + pan;

	for(i=0;i<16;i++)
		Mid_ShortMsg(0xb0+i,0x0a,value);
}

void Mid_LoopFunc(DWORD time)
{
	int			i;
	BOOL		flag = FALSE;
	MID_TRACK	*p;
	DWORDLONG	now  = Mid_Data.playcount2 + (DWORDLONG)Mid_Data.playcount1*Mid_Data.timebase*1000/Mid_Data.tempo;

	if(Mid_Dev.mp     == NULL) return;
	if(Mid_Data.data  == NULL) return;
	if(Mid_Dev_unused.fnmode != MIDFN_MIDLOOP) return;

	Mid_PlayTime += time;

	for(i=0;i<Mid_Data.track;i++){
		p = &(Mid_Data.data[i]);
		if(p->play){
			flag = TRUE;
			while(p->play && p->count<=now)
				Mid_Parse(p);
		}
	}

	Mid_Data.playcount1+=((time*Mid_MulTempo)>>7);

	MidFadeIOFunc();

	if(!flag){
		switch(Mid_Dev_unused.plmode){
			case(MIDPL_NORM):Mid_Init();	break;
			case(MIDPL_STOP):Mid_Stop();	break;
		}
	}
}

static void CALLBACK CBMid_TimeFunc_unused(UINT uID,UINT uMsg,DWORD dwUser,DWORD dw1,DWORD dw2)
{
	BOOL flag = FALSE; // `TRUE` if MIDI file playback reached its end

	CBMid_TimeFunc(uID, uMsg, dwUser, dw1, dw2);

//	Mid_Data.nticks = (Mid_Time.delay*1000 + Mid_Data.fticks) / Mid_Data.tempo;
//	Mid_Data.fticks = Mid_Time.delay*1000 % Mid_Data.tempo;
//	Mid_Data.nticks  = (Mid_Data.fticks + Mid_PlayTime * Mid_Data.timebase * 1000) / Mid_Data.tempo;
//	Mid_Data.fticks += (Mid_Data.timebase * Mid_PlayTime * 1000) - (Mid_Data.nticks * Mid_Data.tempo);

	if(!flag){
		switch(Mid_Dev_unused.plmode){
			case(MIDPL_NORM):Mid_Init();	break;
			case(MIDPL_STOP):Mid_Stop();	break;
		}
	}
}

// playcount1 の取得 //
DWORDLONG Mid_GetPlaycount1(void)
{
	return Mid_Data.playcount1;
}


// playcount2 の取得 //
DWORDLONG Mid_GetPlaycount2(void)
{
	return Mid_Data.playcount2;
}


// 全情報を取得 //
void Mid_GetData(MID_DATA *pData)
{
	*pData = Mid_Data;
}
