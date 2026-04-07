/*
 *   Unused code from `GAMEMAIN.CPP`
 *
 */

#include "unused/PBGMIDI.H"

// お名前入力 //
FVOID NameRegistProc(void)
{
	//char	buf[100],
	//sprintf(buf,"(%2d,%2d)", x, y);
	//GrpPut16(0,0,buf);

/*
	GrpPut16(400,100,temps);
	for(i=0; i<5; i++){
		GrpPut16(100, 100+i*32, ScoreString[i].Score);
		if(CurrentRank == i+1) GrpPut16(85, 100+i*32, "!!");
	}
*/
}

void GameDraw(void);

void GameDraw_unused(void)
{
	GameDraw();
/*
	HDC		hdc;
	char	buf[100];
	MID_DATA	dat;

	if(DxObj.Back->GetDC(&hdc) == DD_OK){
		Mid_GetData(&dat);
		sprintf(buf, "c1 : %I64u", dat.nticks);
		//sprintf(buf, "timebase : %d", dat.timebase);
		TextOut(hdc, 0, 0, buf, strlen(buf));
		//sprintf(buf, "tempo    : %d", dat.tempo);
		//TextOut(hdc, 0, 30, buf, strlen(buf));

		DxObj.Back->ReleaseDC(hdc);
	}
*/
}

extern FBOOL GameReplayInit(int Stage)
{
//	rnd_seed_set(timeGetTime());
}

FVOID GameProc(void)
{
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
}

FVOID WeaponSelectProc(void)
{
	if(IsDraw()){
/*
		HDC		hdc;
		char	buf[100];
		DxObj.Back->GetDC(&hdc);
		sprintf(buf,"Viv.weapon = %d",Viv.weapon);
		TextOut(hdc,0,0,buf,strlen(buf));
		DxObj.Back->ReleaseDC(hdc);

		#ifdef PBG_DEBUG
			OutputDebugMsg();
		#endif
*/
	}
}


FVOID TitleProc(void)
{
/*
	// 鳩プロテクト? //
	if((GetAsyncKeyState(VK_F1) & 0x80000000)
		&& (GetAsyncKeyState(VK_F10) & 0x8000000))
		PostMessage(hWndMain,WM_SYSCOMMAND,SC_CLOSE,0);
*/
}

/*
_inline XAdd(DWORD old,int id)
{
	RndBuf[id] += (random_ref-old);
}
*/
