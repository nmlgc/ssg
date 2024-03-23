/*
 *   Unused snippets from 3D effect code
 *
 */

#include "unused/EFFECT3D.H"

#define TRIANGLE_MAX		50

Triangle3D	Tri[TRIANGLE_MAX];

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
