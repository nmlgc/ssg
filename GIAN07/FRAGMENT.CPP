/*************************************************************************************************/
/*   FRAGMENT.C   破片処理関数群                                                                 */
/*                                                                                               */
/*************************************************************************************************/

#include "FRAGMENT.H"
#include "ssg/internal/RNG.hpp"
#include "hatoyama/logic/coords.h"
#include "hatoyama/logic/ut_math.h"


FRAGMENT_DATA	Fragment[FRAGMENT_MAX];		// 破片データ管理用構造体
int				FragmentPtr = 0;			// 次に破片データを挿入する位置


void fragment_set(int x, int y, uint8_t cmd)
{
	int				i;
	int				l;
	uint8_t d;
	FRAGMENT_DATA	*f = Fragment+FragmentPtr;

	if(cmd==FRG_ESCAPE){
		for(i=0;i<FRAGMENT_MAX;i++){
			f = Fragment+i;
			if(f->count){
				f->vx = ((f->x - x) / 16); // f->count;
				f->vy = ((f->y - y) / 16); // f->count;
			}
		}
	}
	else if(cmd==FRG_APPROACH){
		for(i=0;i<FRAGMENT_MAX;i++){
			f = Fragment+i;
			if(f->count){
				f->vx = (x-f->x)/f->count;
				f->vy = (x-f->y)/f->count;
			}
		}
	}

	f->cmd   = cmd;
	f->x     = x;
	f->y     = y;

	switch(cmd){
		case(FRG_HIT):
			d = (RNG.next() & 0xff);
			l = (PixelToWorld(1) + (RNG.next() % PixelToWorld(3)));
			f->count = 24;
			f->vx    = cosl(d,l);
			f->vy    = sinl(d,l);
		break;

		case(FRG_EVADE):
			d = (RNG.next() & 0xff);
			l = (PixelToWorld(4) + (RNG.next() % PixelToWorld(3)));
			f->count = 24;
			f->vx    = cosl(d,l);
			f->vy    = sinl(d,l);
		break;

		case(FRG_SMOKE):
			f->count = 24;
			f->vx    = 0;
			f->vy    = 0;
		break;

		case(FRG_STAR1):
			f->count = 64;
			d = (RNG.next() & 0xff);
			l = (PixelToWorld(5) + (RNG.next() % PixelToWorld(3)));
			f->vx    = cosl(d,l);
			f->vy    = sinl(d,l);
		break;

		case(FRG_STAR2):
			f->count = 64;
			d = (RNG.next() & 0xff);
			l = (PixelToWorld(4) + (RNG.next() % PixelToWorld(3)));
			f->vx    = cosl(d,l);
			f->vy    = sinl(d,l);
		break;

		case(FRG_STAR3):
			f->count = 64;
			d = (-64 - 48 + (RNG.next() % 96));
			l = (PixelToWorld(6) + (RNG.next() % PixelToWorld(4)));
			f->vx    = cosl(d,l);
			f->vy    = sinl(d,l);
		break;

		case(FRG_HEART):
			f->count = 105;
			d = (RNG.next() & 0xff);
			l = (PixelToWorld(2) + (RNG.next() % PixelToWorld(5)));
			f->vx    = cosl(d,l);
			f->vy    = sinl(d,l);
		break;

		case(FRG_FATCIRCLE):
			f->count = 60;
			f->vx    = 0;
			f->vy    = 0;
		break;

		default:
			// バグだけど... //
		break;
	}

	FragmentPtr = (FragmentPtr+1)%FRAGMENT_MAX;
}

void fragment_move(void)
{
	for(auto& it : Fragment) {
		auto *f = &it;
		if(f->count){
			f->x += f->vx;
			f->y += f->vy;
			f->count--;
		}
	}
}

void fragment_setup(void)
{
	for(auto& it : Fragment) {
		//memset(Fragment+i,0,sizeof(FRAGMENT_DATA));
		it.count = 0;
	}

	FragmentPtr = 0;
}
