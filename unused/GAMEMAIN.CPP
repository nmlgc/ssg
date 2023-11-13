/*
 *   Unused MIDI debugging code
 *
 */

#include "unused/PBGMIDI.H"

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
