/*
 *   Unused particle code
 *
 */

void fragment_set(int x,int y,BYTE cmd)
{
	switch(cmd){
		case(FRG_SMOKE):
			f->count = 24;
			f->vx    = 0;//cosl((BYTE)rnd(),rnd()&0xff);//cosl((BYTE)rnd()%256,64*6);
			f->vy    = 0;//sinl((BYTE)rnd(),rnd()&0xff);//sinl((BYTE)rnd()%256,64*6);
		break;
	}
}