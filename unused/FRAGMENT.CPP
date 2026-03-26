/*
 *   Unused particle code
 *
 */

void fragment_set(int x,int y,BYTE cmd)
{
	FRAGMENT_DATA	*f = Fragment+FragmentPtr;

	switch(cmd){
		case(FRG_SMOKE):
			f->vx    = 0;//cosl((BYTE)rnd(),rnd()&0xff);//cosl((BYTE)rnd()%256,64*6);
			f->vy    = 0;//sinl((BYTE)rnd(),rnd()&0xff);//sinl((BYTE)rnd()%256,64*6);
		break;
	}
}
