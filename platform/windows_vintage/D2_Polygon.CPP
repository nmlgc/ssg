/*                                                                           */
/*   D2_Polygon.cpp   ８ビット時のポリゴン描画                               */
/*                                                                           */
/*                                                                           */

#include "DD_CLIP2D.H"
#include "game/cast.h"
#include "constants.h"
#include "DD_GRP2D.H"


//// [更新履歴] ////
// 2000/05/15       : アセンブラ化で、かろうじてバグ排除



//// [構造体] ////

typedef struct tagLINE_WORK{
	uint64_t	x;	// Upper 32 bits: 開始ｘ座標, lower 32 bits: 誤差変数
	uint32_t	dlx;	// 誤差変数加算値
	uint32_t	d;	// 最小ｘ移動量
} LINE_WORK;



//// [インライン関数] ////

// DDA 構造体のセット //
inline void SetLineWork(LINE_WORK *w, int x1, int x2, int dy)
{
	if(dy == 0) {
		dy++;
	}
	const auto dx = (x2 - x1);
	const auto d = (dx / dy);
	const auto err = (dx % dy);
	const auto dy_unsigned = Cast::sign<std::make_unsigned_t<decltype(dy)>>(dy);
	if(err < 0) {
		w->d = (d - 1);
		w->dlx = -((static_cast<uint64_t>(-err) << 32) / dy_unsigned);
	} else {
		w->d = d;
		w->dlx = ((static_cast<uint64_t>(err) << 32) / dy_unsigned);
	}
	w->x = ((static_cast<uint64_t>(x1) << 32) | 0x80000000);
}


static LINE_WORK TZOID_A,TZOID_B;

static int DrawTrapezoid(int y, int dy);	// 台形の描画



// ポリゴンの描画 //
void GRAPHICS_GEOMETRY_DDRAW::DrawTriangleFan(VERTEX_XY_SPAN<> p)
{
	int		i,j,dy;
	int		nLeft,nRight;
	int		TopY,BottomY;
	VERTEX_XY	temp[100];

	p = Grp_PClip(temp, p);
	const int n = p.size();
	if(n <= 2) {
		return;
	}


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
extern uint32_t	Grp_AddrY[GRP_RES.h];	// Ｙ座標テーブル
extern uint8_t	Col2D;


// 横ラインを引く //
static void GrpHLineX(int x1, int x2, int y)
{
	int temp;
	if(x1>x2) temp = x1,x1 = x2,x2 = temp;
	if(x1<0) x1 = 0;
	if(x2 > (GRP_RES.w - 1)) {
		x2 = (GRP_RES.w - 1);
	}
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

	auto left_x = TZOID_A.x;
	auto det_x  = TZOID_B.x; // - left_x;
	const auto a_d = Cast::up<uint64_t>(TZOID_A.d);
	const auto b_d = Cast::up<uint64_t>(TZOID_B.d);
	const auto left_step = ((a_d << 32) + TZOID_A.dlx);
	const auto det_step  = ((b_d << 32) + TZOID_B.dlx);

	do{
		GrpHLineX((left_x >> 32), (det_x >> 32), y);

		left_x += left_step;
		det_x  += det_step;

		//det_x -= left_x;
		y++;
	}
	while((--dy)>=0);

	TZOID_A.x = left_x;
	TZOID_B.x = det_x;

	return y;
}
