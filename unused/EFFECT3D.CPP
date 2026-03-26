/*
 *   Unused snippets from 3D effect code
 *
 */

#include "unused/EFFECT3D.H"

#define TRIANGLE_MAX		50
#define CIRCLE_MAX			40
//#define CLOUD_MAX			10

Triangle3D	Tri[TRIANGLE_MAX];
Circle3D	Cir[CIRCLE_MAX];
//Cloud2D		Cloud[CLOUD_MAX];

/*
LineList3D	LList_G = {354,39,PList_G,17,PWork_G};
LineList3D	LList_I = {255,39,PList_I,5,PWork_I};
LineList3D	LList_N = {215,39,PList_N,9,PWork_N};
LineList3D	LList_R = {156,39,PList_R,15,PWork_R};
LineList3D	LList_A2 = {96,39,PList_A2,4,PWork_A2};
LineList3D	LList_A1 = {96,39,PList_A1,8,PWork_A1};
LineList3D	LList_W = {32,39,PList_W,11,PWork_W};
*/

FVOID InitTriangle(void)
{
	int		i;

	for(i=0; i<TRIANGLE_MAX; i++){
/*
		Tri[i].Current.z = 64 + i*90*64/TRIANGLE_MAX;
		Tri[i].Current.x = (sinl(i*512/TRIANGLE_MAX,Tri[i].Current.z/32)+320)*64;
		Tri[i].Current.y = (cosl(i*512/TRIANGLE_MAX,Tri[i].Current.z/32)+240)*64;
*/
		Tri[i].Current.x = (rnd()%640)*64;
		Tri[i].Current.y = (rnd()%480)*64;
		Tri[i].Current.z = (rnd()%50+10)*64;

		Tri[i].Speed.x = 0;
		Tri[i].Speed.y = 0;
		Tri[i].Speed.z = 0;

		Tri[i].deg_x = rnd()%256;
		Tri[i].deg_y = rnd()%256;
		Tri[i].deg_z = rnd()%256;

		Tri[i].roll_x = rnd()%6 + 1;
		Tri[i].roll_y = rnd()%6 + 1;
		Tri[i].roll_z = rnd()%6 + 1;
	}

	//return;

	for(i=TRIANGLE_MAX/2; i<TRIANGLE_MAX; i++){
		Tri[i].Current.x = Tri[i-TRIANGLE_MAX/2].Current.x;
		Tri[i].Current.y = Tri[i-TRIANGLE_MAX/2].Current.y;
		Tri[i].Current.z = Tri[i-TRIANGLE_MAX/2].Current.z;

		Tri[i].Speed.x = 0;
		Tri[i].Speed.y = 0;
		Tri[i].Speed.z = 0;

		Tri[i].deg_x = Tri[i-TRIANGLE_MAX/2].deg_x;
		Tri[i].deg_y = Tri[i-TRIANGLE_MAX/2].deg_y;
		Tri[i].deg_z = Tri[i-TRIANGLE_MAX/2].deg_z+128;

		Tri[i].roll_x = Tri[i-TRIANGLE_MAX/2].roll_x;
		Tri[i].roll_y = Tri[i-TRIANGLE_MAX/2].roll_y;
		Tri[i].roll_z = Tri[i-TRIANGLE_MAX/2].roll_z;
	}
}

FVOID MoveTriangle(void)
{
	int		i,j;
	Point3D	temp;

	for(i=0; i<TRIANGLE_MAX; i++){
		Tri[i].deg_x += Tri[i].roll_x;
		Tri[i].deg_y += Tri[i].roll_y;
		Tri[i].deg_z += Tri[i].roll_z;

		for(j=0; j<3; j++){
			temp.x = cosl(j*256/3,Tri[i].Current.z);
			temp.y = sinl(j*256/3,Tri[i].Current.z);
			temp.z = 0;
			RollPoint(&temp,Tri[i].deg_x,Tri[i].deg_y,Tri[i].deg_z);
			Tri[i].Grp[j].x = (temp.x + Tri[i].Current.x)>>6;
			Tri[i].Grp[j].y = (temp.y + Tri[i].Current.y)>>6;
		}
	}
}

FVOID DrawTriangle(void)
{
	int		i;
	POINT	*p;

	GrpLock();

	for(i=0; i<TRIANGLE_MAX; i++){
		p = Tri[i].Grp;
		GrpSetColor(2,2,5);
		GrpLine(p[0].x,p[0].y,p[1].x,p[1].y);
		GrpSetColor(3,3,5);
		GrpLine(p[1].x,p[1].y,p[2].x,p[2].y);
		GrpSetColor(4,4,5);
		GrpLine(p[2].x,p[2].y,p[0].x,p[0].y);
	}

	GrpUnlock();
}


FVOID InitCircle(void)
{
	int		i;

	for(i=0; i<CIRCLE_MAX; i++){
		Cir[i].deg = rnd()%256;
		Cir[i].n   = 16;
		Cir[i].ox  = (rnd()%640)*64;
		Cir[i].oy  = (rnd()%480)*64;
		Cir[i].r   = (rnd()%30+30)*64;
	}
}

FVOID MoveCircle(void)
{
	int		i;

	for(i=0; i<CIRCLE_MAX; i++){
		Cir[i].deg += 3;
	}
}

FVOID DrawCircle(void)
{
	int		i,j;
	int		x,y,l;
	BYTE	deg;

	GrpLock();

	for(i=0; i<CIRCLE_MAX; i++){
		for(j=0; j<Cir[i].n; j++){
			deg = Cir[i].deg+j*256/Cir[i].n;
			if(deg<128) continue;
			x = (Cir[i].ox + cosl(deg,Cir[i].r))>>6;
			y = (Cir[i].oy + sinl(deg,Cir[i].r))>>6;
			l = j*3+(Cir[i].r>>7);

			GrpSetColor(4,4,5);
			GrpLine(x,y,x,y-l);
			y-=l;l -= l/4;
			GrpSetColor(3,3,5);
			GrpLine(x,y,x,y-l);
			y-=l;l -= l/4;
			GrpSetColor(2,2,5);
			GrpLine(x,y,x,y-l);
		}
	}
	for(i=0; i<CIRCLE_MAX; i++){
		for(j=0; j<Cir[i].n; j++){
			deg = Cir[i].deg+j*256/Cir[i].n;
			if(deg>128) continue;
			x = (Cir[i].ox + cosl(deg,Cir[i].r))>>6;
			y = (Cir[i].oy + sinl(deg,Cir[i].r))>>6;
			l = j*3+(Cir[i].r>>7);

			GrpSetColor(4,4,5);
			GrpLine(x,y,x,y-l);
			y-=l;l -= l/4;
			GrpSetColor(3,3,5);
			GrpLine(x,y,x,y-l);
			y-=l;l -= l/4;
			GrpSetColor(2,2,5);
			GrpLine(x,y,x,y-l);
		}
	}

	GrpUnlock();
}


void MoveLineList3D(WireFrame3D *w)
{
	int			i,j;
	//static BYTE		count = 0;
	//BOOL	bEnableRoll = FALSE;

	//if(count) count--;
	//else bEnableRoll = TRUE;

	for(j=0; j<(w->n); j++){
/*
		if(bEnableRoll){
			w->LList[j].DegX+=2;
			w->LList[j].DegY+=1;
			w->LList[j].DegZ+=4;
			if(w->LList[j].DegX==0) count = 64;
		}
*/
		for(i=0; i<(w->LList[j].n); i++){
			// ...
		}
	}
}

/*
// ３面の雲の初期化を行う //
FVOID InitStg3Cloud(void)
{
	Cloud2D		*p;		//Cloud[CLOUD_MAX];
	int			i;

	p = Cloud;

	for(i=0; i<CLOUD_MAX; i++, p++){
		if(rnd()&1) p->x = 128*64+(rnd()>>1)%(100*64);
		else        p->x = 512*64-(rnd()>>1)%(100*64);
		p->y    = ((i*680*64)/CLOUD_MAX)-200*64;	//GY_RND;

		p->type = (rnd()>>2)%5;
		if(p->type == 2) p->type = 5;

		p->vy   = rnd()%(64 * 6) + 64 * 11;
	}
}


// ３面の雲を動作させる //
FVOID MoveStg3Cloud(void)
{
	Cloud2D		*p;		//Cloud[CLOUD_MAX];
	int			i;

	p = Cloud;

	for(i=0; i<CLOUD_MAX; i++, p++){
		p->y += p->vy;

		if(p->y > (480+200)*64){
			if(rnd()&1) p->x = 128*64+(rnd()>>1)%(100*64);
			else        p->x = 512*64-(rnd()>>1)%(100*64);
			p->y    = -200*64;

			p->type = (rnd()>>2)%5;
			if(p->type == 2) p->type = 5;

			p->vy   = rnd()%(64 * 6) + 64 * 11;
		}
	}
}


// ３面の雲を描画する //
FVOID DrawStg3Cloud(void)
{
#define _RsetMacro(x, y, w, h)		{(x), (y), ((x)+(w)), ((y)+(h))}
	static RECT Data[6] = {
		_RsetMacro(  0, 288, 144, 160),			// Large_1
		_RsetMacro(144, 288, 144, 112),			// Large_2
		_RsetMacro(288, 288, 144, 176),			// Large_3

		//_RsetMacro(480, 288,  32,  48),			// Small_1
		_RsetMacro(144, 400,  32,  48),			// Small_2
		_RsetMacro(176, 400,  48,  32),			// Small_3
		_RsetMacro(224, 400,  48,  48),			// Small_4
	};
#undef _RsetMacro

	static POINT	Size[6] = {
		144/2, 160/2, 144/2, 112/2, 144/2, 176/2,		// Large
		 //32,  48,
		 32/2,  48/2,  48/2,  32/2,  48/2,  48/2		// Small
	};

	RECT		src;
	Cloud2D		*p;		//Cloud[CLOUD_MAX];
	int			i, j;
	int			x, y;

	p = Cloud;

	for(i=0; i<CLOUD_MAX; i++, p++){
		j   = p->type;
		x   = (p->x >> 6) - Size[j].x;
		y   = (p->y >> 6) - Size[j].y;
		src = Data[j];

		GrpBlt(&src, x, y, GrEnemy);
	}
}
*/

FVOID MoveStg4Rock(void)
{
	int		i, y;
	Rock3D	*p = Rock;

	for(i=0; i<ROCK_MAX; i++, p++){
		switch(p->State){
		case(STG4ROCK_ACCMOVE2):
/*			p->v += p->a;
			p->x += cosl(p->d, p->v);
			p->y += (p->vy + sinl(p->d, p->v));

			if(p->count > 60){
				if(p->y > (250+40)*64 || p->y < (-250-40)*64){
					//p->x     = (rnd()%(500*64)-250*64);
					y = (i%4)*dy + (rnd()%dy2);
					p->x = (rnd()%(500*64)-250*64);		//
					p->y = -250*64-y;						// 上の方なのだ

					p->vy    = (4-Rock[i].GrpID)*32 * 3;
					p->v     = p->vy;
					p->a     = 0;
					p->State = STG4ROCK_ACCMOVE1;
				}
				else{
					p->v = p->vy = (4-Rock[i].GrpID)*32 * 3;
					p->a = 0;
					p->State = STG4ROCK_ACCMOVE1;
				}

				break;
			}

			if(p->y > (250+40)*64 || p->y < (-250-40)*64){
				p->x     = (rnd()%(700*64)-350*64);
				p->y     = (250+40)*64;
				p->vy    = (4-Rock[i].GrpID)*32 * 3;
				p->v     = 10;
				p->a     = -4;
			}*/
		break;
		}
	}
}
