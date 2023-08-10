/*                                                                           */
/*   D2_Polygon.cpp   ８ビット時のポリゴン描画                               */
/*                                                                           */
/*                                                                           */

#include "D2_Polygon.H"
#include "DD_CLIP2D.H"
#include <string.h>


static LINE_WORK TZOID_A,TZOID_B;

static int DrawTrapezoid(int y, int dy);	// 台形の描画



// ポリゴンの描画 //
void _2DPolygon(WINDOW_POINT *p, int n)
{
	int		i,j,dy;
	int		nLeft,nRight;
	int		TopY,BottomY;
	WINDOW_POINT	temp[100];

	switch(i=Grp_PClip(temp,p,n)){
		case(0):			// 完全に外側
		return;

		case(-1):			// 完全に内側
		break;

		default:			// クリッピングがかかった
			n = i;
			p = temp;
		break;
	}

	if(n<=2) return;


	// 頂点サーチ //
	TopY = BottomY = p[n-1]. y;
	nLeft = nRight = n-1;
	for(i=n-2; i>=0; i--){
		if(p[i].y == TopY){
			nLeft = i;
		}
		else if(p[i].y > TopY){
			// 一番下にある点のＹ座標を更新
			if(BottomY < p[i].y) BottomY = p[i].y;
		}
		else{
			// 一番上にある点のＹ座標を更新
			nRight = i;
			nLeft  = i;
			TopY   = p[i].y;
		}
	}

	if(nRight==0 && nLeft==n-1){
		nLeft  = 0;
		nRight = n-1;
	}

	//if(nLeft==nRight) n2 = n-1;
	//else              n2 = n-2;

	i = (nLeft-1 < 0) ? (n-1) : nLeft-1;
	SetLineWork(&TZOID_A, p[nLeft ].x, p[i].x, p[i].y-p[nLeft ].y);
	j = (nRight+1 > n-1) ? 0 : nRight+1;
	SetLineWork(&TZOID_B, p[nRight].x, p[j].x, p[j].y-p[nRight].y);

	// 描画ループ //
	while(TopY<BottomY/*n2>0*/){
		//i = (nLeft-1 < 0) ? (n-1) : nLeft-1;
		//j = (nRight+1 > n-1) ? 0 : nRight+1;

		if(p[i].y < p[j].y)	dy = p[i].y-TopY;
		else				dy = p[j].y-TopY;

		//if(Key_Data&KEY_TAMA) return;

		TopY = DrawTrapezoid(TopY,dy);


		if(p[i].y <= TopY){
			// 左側(TZOID_A)を更新する必要がある場合 //
			nLeft = i;//(nLeft-1 < 0) ? (n-1) : nLeft-1;
			i = (nLeft-1 < 0) ? (n-1) : nLeft-1;
			SetLineWork(&TZOID_A, p[nLeft ].x, p[i].x, p[i].y-p[nLeft ].y);
			//n2--;
		}
		if(p[j].y <= TopY){
			// 右側(TZOID_B)を更新する必要がある場合 //
			nRight = j;//(nRight+1 > n-1) ? 0 : nRight+1;
			j = (nRight+1 > n-1) ? 0 : nRight+1;
			SetLineWork(&TZOID_B, p[nRight].x, p[j].x, p[j].y-p[nRight].y);
			//n2--;
		}
	}
}

extern uint8_t*	Grp_Vram;	// これは公開しない
extern long				Grp_Pitch;				// これも公開しない
extern uint32_t	Grp_AddrY[480];	// Ｙ座標テーブル
extern uint32_t	Col2D;


// 横ラインを引く //
static void GrpHLineX(int x1, int x2, int y)
{
	int temp;
	if(x1>x2) temp = x1,x1 = x2,x2 = temp;
	if(x1<0) x1 = 0;
	if(x2>639) x2 = 639;
	//if(y<0 || y>479) return;
	//if(y<0) y = 0;
	//if(y>479) y = 479;

	memset((Grp_Vram + Grp_AddrY[y] + x1), Col2D, ((x2 - x1) + 1));
}

// 台形の描画 //
int DrawTrapezoid(int y, int dy)
{
	// NULL チェック(高速化のために外す？)
	if(!Grp_Vram) {
		return y;
	}

	int		left_x = TZOID_A.x;
	int		det_x  = TZOID_B.x;// - left_x;

	do{
		GrpHLineX(left_x,det_x,y);

		_asm{
			//MOV		EAX,TZOID_B.dlx
			//ADD		TZOID_B.s,EAX
			MOV		EAX,TZOID_B.s
			ADD		EAX,TZOID_B.dlx
			MOV		TZOID_B.s,EAX

			MOV		EAX,TZOID_B.x
			ADC		EAX,TZOID_B.d
			MOV		TZOID_B.x,EAX
			MOV		det_x,EAX

			//MOV		EAX,TZOID_A.dlx
			//ADD		TZOID_A.s,EAX
			MOV		EAX,TZOID_A.s
			ADD		EAX,TZOID_A.dlx
			MOV		TZOID_A.s,EAX

			MOV		EAX,left_x
			ADC		EAX,TZOID_A.d
			MOV		left_x,EAX
		}

		//det_x -= left_x;
		y++;
	}
	while((--dy)>=0);

	TZOID_A.x = left_x;

	return y;
}
