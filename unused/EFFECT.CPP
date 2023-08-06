/*
 *   Unused effect code
 *
 */

#include "EFFECT.H"
#include "GIAN07/EFFECT.H"
#include "GIAN07/FONTUTY.H"
#include "game/coords.h"

void SEffectMove(void)
{
	for(const auto& e : SEffect) {
		switch(e.cmd) {
		case(SEFC_GAMEOVER2):
		if(e.time == 0) {
			e.cmd  = SEFC_GAMEOVER3;
			e.time = 180;
		}
		break;

		case(SEFC_GAMEOVER3):
		if(e.time == 0) {
			e.cmd = SEFC_NONE;
		}
		break;
		}
	}
}

void SEffectDraw(void)
{
	char	buf[20];

	for(const auto& e : SEffect) {
		switch(e.cmd) {
		case(SEFC_GAMEOVER3):
			strcpy(buf,"GAME OVER");
			// if((e.time % 60) < 30) {
			// 	break;
			// }
			for(auto j = 0; j < 9; j++) {
				// *37 //
				const auto x = ((e.x >> WORLD_COORD_BITS) + ((j - 4) * 35));
				const auto y = (e.y >> WORLD_COORD_BITS);
				GrpPutc(x, y, buf[j]);
			}
			break;
		}
	}
}
