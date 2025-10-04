/*                                                                           */
/*   Effect3D.cpp   ３Ｄエフェクトの処理                                     */
/*                                                                           */
/*                                                                           */

#include "EFFECT3D.H"
#include "GIAN.H"
#include "game/cast.h"
#include "game/ut_math.h"
#include "platform/graphics_backend.h"

#define CIRCLE_MAX			40
#define CUBE_MAX			8
#define STAR_MAX			40
#define FAKE_ECLSTR_MAX		80
#define ROCK_MAX			28
//#define CLOUD_MAX			10


Circle3D	Cir[CIRCLE_MAX];
Cube3D		Cube[CUBE_MAX];
Star2D		Star[STAR_MAX];
Rock3D		Rock[ROCK_MAX];
//Cloud2D		Cloud[CLOUD_MAX];

WFLine2D		WFLine;
FakeECLString	FakeECLStr[FAKE_ECLSTR_MAX];

#define _ PIXEL_POINT

WORLD_POINT PList_W[11] = {
	_{  0, 15 }, _{ 15, 66 }, _{ 32, 47 }, _{ 48, 66 },
	_{ 63, 14 }, _{ 52, 11 }, _{ 42, 38 }, _{ 32, 26 },
	_{ 21, 38 }, _{ 11, 10 }, _{  0, 15 },
};

WORLD_POINT PList_A1[8] = {
	_{  96, 12 }, _{  66, 61 }, _{  75, 67 }, _{ 83, 56 },
	_{ 107, 56 }, _{ 115, 67 }, _{ 125, 61 }, _{ 96, 12 },
};

WORLD_POINT PList_A2[4] = {
	_{ 96, 34 }, _{ 90, 44 }, _{ 101, 44 }, _{ 96, 34 },
};

WORLD_POINT PList_R[15-1] = {
	_{ 132, 14 }, _{ 132, 64 }, _{ 145, 64 }, _{ 145, 27 },
	_{ 164, 27 },

	//_{ 150, 41 }, _{ 150, 43 },
	_{ 150, 42 },

	_{ 171, 66 },
	_{ 173, 66 }, _{ 181, 57 }, _{ 167, 43 }, _{ 180, 29 },
	_{ 180, 27 }, _{ 170, 14 }, _{ 132, 14 },
};

WORLD_POINT PList_N1[9] = {
	_{ 189, 12 }, _{ 189, 64 }, _{ 201, 64 }, _{ 201, 40 },
	_{ 239, 66 }, _{ 239, 14 }, _{ 227, 14 }, _{ 227, 38 },
	_{ 189, 12 },
};

WORLD_POINT PList_N2[9] = {
	_{ 189, 12 }, _{ 189, 64 }, _{ 201, 64 }, _{ 201, 40 },
	_{ 239, 66 }, _{ 239, 14 }, _{ 227, 14 }, _{ 227, 38 },
	_{ 189, 12 },
};

WORLD_POINT PList_I[5] = {
	_{ 248, 14 }, _{ 248, 64 }, _{ 262, 64 }, _{ 262, 14 },
	_{ 248, 14 },
};

WORLD_POINT PList_G[17] = {
	_{ 354, 11 }, _{ 328, 22 }, _{ 328, 57 }, _{ 354, 68 },
	_{ 380, 59 }, _{ 380, 34 }, _{ 355, 34 }, _{ 354, 45 },
	_{ 367, 46 }, _{ 367, 51 }, _{ 355, 55 }, _{ 342, 50 },
	_{ 342, 29 }, _{ 354, 24 }, _{ 372, 30 }, _{ 377, 19 },
	_{ 354, 11 },
};

#undef _

/*
LineList3D	LList_G = {354,39,PList_G,17,PWork_G};
LineList3D	LList_I = {255,39,PList_I,5,PWork_I};
LineList3D	LList_N = {215,39,PList_N,9,PWork_N};
LineList3D	LList_R = {156,39,PList_R,15,PWork_R};
LineList3D	LList_A2 = {96,39,PList_A2,4,PWork_A2};
LineList3D	LList_A1 = {96,39,PList_A1,8,PWork_A1};
LineList3D	LList_W = {32,39,PList_W,11,PWork_W};
*/

LineList3D Warning[8] = {
	{ { 192, 39 }, PList_W },
	{ { 192, 39 }, PList_A1 },
	{ { 192, 39 }, PList_A2 },
	{ { 192, 39 }, PList_R },
	{ { 192, 39 }, PList_N1 },
	{ { 192, 39 }, PList_I },
	{ { (192 - (296 - 215)), 39 }, PList_N2 },
	{ { 192, 39 }, PList_G }
};


static void RollPoint(Point3D *p, uint8_t dx, uint8_t dy, uint8_t dz);
static void __Draw3DCube(const Cube3D *c); // 汎用３Ｄキューブ描画

void Transform3D(Point3D *p, uint8_t dx, uint8_t dy, uint8_t dz)
{
	static Point3D	temp;

	temp.y = p->y;
	temp.z = p->z;
	p->y   = (cosl(dx, temp.y) - sinl(dx, temp.z));
	p->z   = (sinl(dx, temp.y) + cosl(dx, temp.z));

	temp.x = p->x;
	temp.z = p->z;
	p->x   = ( cosl(dy, temp.x) + sinl(dy, temp.z));
	p->z   = (-sinl(dy, temp.x) + cosl(dy, temp.z));

	temp.x = p->x;
	temp.y = p->y;
	p->x   = (cosl(dz, temp.x) - sinl(dz, temp.y));
	p->y   = (sinl(dz, temp.x) + cosl(dz, temp.y));
}

void ShiftRight6Bit(const Point3D *o, Point3D *p)
{
	p->x = (((p->x + o->x) >> 6) + 320);
	p->y = (((p->y + o->y) >> 6) + 240);
}



void InitWarning(void)
{
	static bool bInitialized = false;

	if(bInitialized) return;

	InitLineList3D(Warning);
	bInitialized = true;
}

void DrawWarning(void)
{
	constexpr PIXEL_LTRB src = { 0, (152 + 16), 384, (232 + 16) };
	int	st,det;
	static int count;

	count+=8;

	if(Warning[0].DegX == 0) {
		GrpGeom->Lock();
		GrpGeom->SetAlphaNorm(Cast::down_sign<uint8_t>(128 + sinl(count, 48)));
		GrpGeom->SetColor({ 5, 0, 0 });
		GrpGeom->DrawBoxA(129, (40 + 6), (128 + 384), (60 + 6));
		GrpGeom->DrawBoxA(
			129, (60 + (232 - 152) - 4), (128 + 384), (80 + (232 - 152) - 4)
		);
		GrpGeom->Unlock();
		GrpSurface_Blit({ (128 + 1), (60 + 1) }, SURFACE_ID::SYSTEM, src);
	}
	else{
		GrpGeom->Lock();

		if(Warning[0].DegX < 10) {
			st=0;
			det=0;
			st  = 0;
			det = 0;
		} else if(Warning[0].DegX < 20) {
			st=-4;
			det=1;
		}
		else{
			//if(Warning[0].DegX < 40){
			st  = -8;
			det = 2;
		}

		constexpr auto w = std::span(Warning);
		GrpGeom->SetColor({ 1, 1, 5 });	MoveWarningR(st); 	DrawLineList3D(w);
		GrpGeom->SetColor({ 2, 2, 5 });	MoveWarningR(det);	DrawLineList3D(w);
		GrpGeom->SetColor({ 3, 3, 5 });	MoveWarningR(det);	DrawLineList3D(w);
		GrpGeom->SetColor({ 4, 4, 5 });	MoveWarningR(det);	DrawLineList3D(w);
		GrpGeom->SetColor({ 5, 5, 5 });	MoveWarningR(det);	DrawLineList3D(w);

		GrpGeom->Unlock();
	}
}

void MoveWarningR(char count)
{
	if(count==0) return;

	for(auto& llist : Warning) {
		llist.DegX += (count * 2);
		llist.DegY += (count * 1);
		llist.DegZ += (count * 4);
	}
}

void MoveWarning(uint8_t count)
{
	for(auto& llist : Warning) {
		llist.DegX = ((count < 64) ? ((64 - count) * 2) : 0);
		llist.DegY = ((count < 64) ? ((64 - count) * 1) : 0);
		llist.DegZ = ((count < 64) ? ((64 - count) * 4) : 0);
	}
}

void InitLineList3D(std::span<LineList3D> w)
{
	for(auto& llist : w) {
		for(auto& point : llist.p) {
			point -= llist.center;
		}
	}
}

void DrawLineList3D(std::span<const LineList3D> w)
{
	const auto roll = [](const LineList3D& llist, const WORLD_POINT& point) {
		Point3D temp = { point.x, point.y, 0 };
		RollPoint(&temp, llist.DegX, llist.DegY, llist.DegZ);
		return WORLD_POINT{ &temp.x, &temp.y };
	};

	for(const auto& llist : w) {
		WORLD_POINT line_p[2] = { roll(llist, llist.p.front()) };
		for(const auto& point : (llist.p | std::views::drop(1))) {
			line_p[1] = roll(llist, point);
			const auto p1 = (PIXEL_POINT{ 320, 100 } + line_p[0].ToPixel());
			const auto p2 = (PIXEL_POINT{ 320, 100 } + line_p[1].ToPixel());
			GrpGeom->DrawLine(p1.x, p1.y, p2.x, p2.y);
			std::swap(line_p[1], line_p[0]);
		}
	}
}

static void RollPoint(Point3D *p, uint8_t dx, uint8_t dy, uint8_t dz)
{
	Point3D		temp;

	temp.y = p->y;
	temp.z = p->z;
	p->y = cosl(dx,temp.y) - sinl(dx,temp.z);
	p->z = sinl(dx,temp.y) + cosl(dx,temp.z);

	temp.x = p->x;
	temp.z = p->z;
	p->x =   cosl(dy,temp.x) + sinl(dy,temp.z);
	p->z = - sinl(dy,temp.x) + cosl(dy,temp.z);

	temp.x = p->x;
	temp.y = p->y;
	p->x = cosl(dz,temp.x) - sinl(dz,temp.y);
	p->y = sinl(dz,temp.x) + cosl(dz,temp.y);
}


void Init3DCube(void)
{
	int		i;

	for(i=0; i<CUBE_MAX; i++){
		Cube[i].l = 30*64;
		Cube[i].d.dx = rnd();
		Cube[i].d.dy = rnd();
		Cube[i].d.dz = rnd();
		Cube[i].p.x = cosl(i*256/CUBE_MAX, 200*64);
		Cube[i].p.y = sinl(i*256/CUBE_MAX, 200*64);
		Cube[i].p.z = 0;
	}

	for(auto& it : Star) {
		it.x = ((rnd() % (640 - 256)) + 128);
		it.y = -(rnd() % 480);
		it.vy = ((rnd() % 10) + 10);
	}
}


void Draw3DCube(void)
{
	for(const auto& it : Star) {
		constexpr PIXEL_LTWH rc = { 136, 272, 16, 24 };
		GrpSurface_Blit({ it.x, it.y }, SURFACE_ID::SYSTEM, rc);
	}

	GrpGeom->Lock();
	for(const auto& it : Cube) {
		__Draw3DCube(&it);
	}
	GrpGeom->Unlock();
}


void Move3DCube(void)
{
	int				i;
	int				l,d2;
	static uint16_t d;
	static uint16_t dx, dy, dz;

	d+=64*4;

	dx+=32*4;
	dy-=16*4;

	d2 = sinl(d>>8, 512/CUBE_MAX);
	l = sinl(d>>7, 100*64)+(200-20)*64;


	for(i=0; i<CUBE_MAX; i++){
		Cube[i].l = 15*64 + (l>>4) + i*128;
		Cube[i].d.dx+=4;
		Cube[i].d.dy-=4;
		//Cube[i].p.x = cosl(i*256/CUBE_MAX+d2, l);
		//Cube[i].p.y = sinl(i*256/CUBE_MAX+d2, l);
		Cube[i].p.x = cosl(i*500/CUBE_MAX+d2, l);
		Cube[i].p.y = sinl(i*500/CUBE_MAX+d2, l);
		Cube[i].p.z = (i-CUBE_MAX/2)*64*40;
		Transform3D(&Cube[i].p,dx>>8,dy>>8,dz>>8);
	}

	for(auto& it : Star) {
		it.y += it.vy;
		if(it.y > 480) {
			it.x = ((rnd() % (640 - 256)) + 128);
			it.y = 0;
			it.vy = ((rnd() % 10) + 10);
		}
	}
}


// 汎用３Ｄキューブ描画 //
static void __Draw3DCube(const Cube3D *c)
{
	// ３Ｄで透過色付きorアルファ比較ポリゴンを使えばもっともっと早いのだが... //
	// ８ビット対応のため、まぁ仕方が無いか... //

	int			x,y,z;
	int			l,l2;
	Point3D		p1,p2;
	Point3D		o;

	o  = c->p;
	l  = c->l;
	const uint8_t dx = c->d.dx;
	const uint8_t dy = c->d.dy;
	const uint8_t dz = c->d.dz;

	l2  = l;

	// GrpGeom->SetColor({ 1, 1, 2 });
	GrpGeom->SetColor({ 1, 1, 3 });
	for(x=-1; x<=1; x++){
		for(y=-1; y<=1; y++){
			p1.x = x * l;
			p1.y = y * l;
			p1.z =    -l2;
			Transform3D(&p1, dx, dy, dz);
			ShiftRight6Bit(&o,&p1);

			p2.x = x * l;
			p2.y = y * l;
			p2.z =     l2;
			Transform3D(&p2, dx, dy, dz);
			ShiftRight6Bit(&o,&p2);

			GrpGeom->DrawLine(p1.x, p1.y, p2.x, p2.y);
		}
	}

	GrpGeom->SetColor({ 0, 0, 3 });
	for(y=-1; y<=1; y++){
		for(z=-1; z<=1; z++){
			p1.x =    -l2;
			p1.y = y * l;
			p1.z = z * l;
			Transform3D(&p1, dx, dy, dz);
			ShiftRight6Bit(&o,&p1);

			p2.x =     l2;
			p2.y = y * l;
			p2.z = z * l;
			Transform3D(&p2, dx, dy, dz);
			ShiftRight6Bit(&o,&p2);

			GrpGeom->DrawLine(p1.x, p1.y, p2.x, p2.y);
		}
	}

	// GrpGeom->SetColor({ 1, 1, 3 });
	GrpGeom->SetColor({ 1, 1, 4 });
	for(x=-1; x<=1; x++){
		for(z=-1; z<=1; z++){
			p1.x = x * l;
			p1.y =    -l2;
			p1.z = z * l;
			Transform3D(&p1, dx, dy, dz);
			ShiftRight6Bit(&o,&p1);

			p2.x = x * l;
			p2.y =     l2;
			p2.z = z * l;
			Transform3D(&p2, dx, dy, dz);
			ShiftRight6Bit(&o,&p2);

			GrpGeom->DrawLine(p1.x, p1.y, p2.x, p2.y);
		}
	}
}


void InitEffectFakeECL(void)
{
	int v;

	WFLine.d  = 0;
	WFLine.ox = 640*64/2;
	WFLine.oy = 480*64/2;
	WFLine.w  = 30;

	for(auto& it : FakeECLStr) {
		const uint8_t d = (rnd() % 128);
		v = rnd()%(64*5) + 64*5;

		it.SrcX = (((rnd() % 7) *  9) * 8);
		it.SrcY = ((rnd() % 16) * 16);
		it.x    = ((28 + (rnd() % 484)) * 64);
		it.y    = -((rnd() % 640) * 64);
		it.vx   = 0; //cosl(d, v);
		it.vy   = v; //sinl(d, v);
	}
}

void MoveEffectFakeECL(void)
{
	int v;

	WFLine.ox = (WFLine.ox+1)%64;
	WFLine.oy = (WFLine.oy+62)%64;

	for(auto& it : FakeECLStr) {
		it.x += it.vx;
		it.y += it.vy;

		if(it.y >= (480 * 64)) {
			const uint8_t d = (rnd() % 128);
			v = rnd()%(64*5) + 64*5;

			it.SrcX = (((rnd() % 7) *  9) * 8);
			it.SrcY = ((rnd() % 16) * 16);
			it.x    = ((28 + (rnd() % 484)) * 64);
			it.y    = -((rnd() % 640) * 64);
			it.vx   = 0; //cosl(d, v);
			it.vy   = v; //sinl(d, v);
		}
	}
}

void DrawEffectFakeECL(void)
{
	PIXEL_LTRB	src;
	int		i,j;

	GrpGeom->Lock();

	// GrpGeom->SetColor({ 3, 2, 0 });	// 後半戦用
	GrpGeom->SetColor({ 0, 2, 0 });
	// GrpGeom->SetColor({ 0, 0, 3 });

	for(i=128-WFLine.ox/2; i<640-128; i+=32)
		GrpGeom->DrawLine(i, 0, i, 480);

	for(j=WFLine.oy/2; j<480; j+=32)
		GrpGeom->DrawLine(128, j, (640 - 128), j);


	// GrpGeom->SetColor({ 5, 3, 0 });	// 後半戦用
	GrpGeom->SetColor({ 0, 3, 0 });
	// GrpGeom->SetColor({ 0, 0, 4 });

	for(i=128-WFLine.ox; i<640-128; i+=64)
		GrpGeom->DrawLine(i, 0, i, 480);

	for(j=-WFLine.oy; j<480; j+=64)
		GrpGeom->DrawLine(128, j, (640 - 128), j);

	GrpGeom->Unlock();

	for(const auto& it : FakeECLStr) {
		src = PIXEL_LTWH{it.SrcX, it.SrcY, 72, 16};
		GrpSurface_Blit({ (it.x >> 6), (it.y >> 6) }, SURFACE_ID::MAPCHIP, src);
	}

	src = { 0, 272, 416, 352 };
	GrpSurface_Blit({ 128, 400 }, SURFACE_ID::MAPCHIP, src);
}

/*
// ３面の雲の初期化を行う //
void InitStg3Cloud(void)
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
void MoveStg3Cloud(void)
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
void DrawStg3Cloud(void)
{
#define _RsetMacro(x, y, w, h)		{(x), (y), ((x)+(w)), ((y)+(h))}
	static PIXEL_LTRB Data[6] = {
		_RsetMacro(  0, 288, 144, 160),			// Large_1
		_RsetMacro(144, 288, 144, 112),			// Large_2
		_RsetMacro(288, 288, 144, 176),			// Large_3

		//_RsetMacro(480, 288,  32,  48),			// Small_1
		_RsetMacro(144, 400,  32,  48),			// Small_2
		_RsetMacro(176, 400,  48,  32),			// Small_3
		_RsetMacro(224, 400,  48,  48),			// Small_4
	};
#undef _RsetMacro

	static PIXEL_LTRB	Size[6] = {
		144/2, 160/2, 144/2, 112/2, 144/2, 176/2,		// Large
		 //32,  48,
		 32/2,  48/2,  48/2,  32/2,  48/2,  48/2		// Small
	};

	PIXEL_LTRB	src;
	Cloud2D		*p;		//Cloud[CLOUD_MAX];
	int			i, j;
	int			x, y;

	p = Cloud;

	for(i=0; i<CLOUD_MAX; i++, p++){
		j   = p->type;
		x   = (p->x >> 6) - Size[j].x;
		y   = (p->y >> 6) - Size[j].y;
		src = Data[j];

		GrpSurface_Blit({ x, y }, SURFACE_ID::ENEMY, src);
	}
}
*/

void InitStg4Rock(void)
{
	int		i;
	int		id = 2;
	int		y  = 0;
	constexpr int dy = (500 * (64 / 4));
	constexpr int dy2 = (dy / 2);

	for(i=0; i<ROCK_MAX; i++){
		y = (i%4)*dy + (rnd()%dy2);
			//((380*64/16) * (i%(ROCK_MAX/16+1))) + rnd()%(380*64/16);

		Rock[i].x = (rnd()%(500*64)-250*64);		//
		Rock[i].y = -250*64-y;						// 上の方なのだ
		Rock[i].z = (rnd()%(500*64)-250*64);		//

		if(i == ROCK_MAX*5/8) id--;
		if(i == ROCK_MAX*7/8) id--;

		Rock[i].GrpID = id;

		Rock[i].vx    = 0;
		Rock[i].vy    = (4-Rock[i].GrpID)*16;
		Rock[i].v     = Rock[i].vy;

		Rock[i].count = 0;
		Rock[i].a     = 0;
		Rock[i].d     = 64;
		Rock[i].State = STG4ROCK_STDMOVE;
	}
}


void MoveStg4Rock(void)
{
	constexpr int dy = (500 * (64 / 4));
	constexpr int dy2 = (dy / 2);

	for(auto& it : Rock) {
		auto *p = &it;
		p->count++;

		switch(p->State){
		case(STG4ROCK_STDMOVE):
			p->y += p->vy;
			if(p->y > (250+40)*64){
				p->x     = (rnd()%(500*64)-250*64);
				p->y     = (-250-40-rnd()%250)*64;
				p->vy    = ((4 - p->GrpID) * 16);
				p->v     = p->vy;
			}
		break;

		case(STG4ROCK_ACCMOVE1):
			p->v += p->a;
			p->vy = p->v;
			p->y += p->vy;

			if(p->y > (250+40)*64){
				p->x     = (rnd()%(500*64)-250*64);
				p->y     = (-250-40-rnd()%250)*64;
				p->vy    = ((4 - p->GrpID) * 32 * 3);
				p->v     = p->vy;
				p->a     = 0;
			}
		break;

		case(STG4ROCK_ACCMOVE2):
			p->v -= p->a;
			p->vy = p->v;
			p->y += p->vy;

			if(p->count > 60*2){
				if(p->y > (250+40)*64 || p->y < (-250-40)*64){
					p->x     = (rnd()%(500*64)-250*64);
					p->y     = (-250-40-rnd()%250)*64;
					p->vy    = ((4 - p->GrpID) * 32);
					p->v     = p->vy;
				}

				p->vy    = ((4 - p->GrpID) * 32);
				p->a     = 2;
				p->v     = p->vy;
				p->State = STG4ROCK_ACCMOVE1;

				break;
			}

			if(p->y > (250+40)*64 || p->y < (-250-40)*64){
				p->x     = (rnd()%(500*64)-250*64);
				p->y     = (250+40)*64+rnd()%250;
				p->vy    = (-(4 - p->GrpID) * 32 * 3);
				p->v     = p->vy;
				p->a     = 0;
			}

/*			p->v += p->a;
			p->x += cosl(p->d, p->v);
			p->y += (p->vy + sinl(p->d, p->v));

			if(p->count > 60){
				if(p->y > (250+40)*64 || p->y < (-250-40)*64){
					//p->x     = (rnd()%(500*64)-250*64);
					y = (i%4)*dy + (rnd()%dy2);
					p->x = (rnd()%(500*64)-250*64);		//
					p->y = -250*64-y;						// 上の方なのだ

					p->vy    = ((4 - p->GrpID) * 32 * 3);
					p->v     = p->vy;
					p->a     = 0;
					p->State = STG4ROCK_ACCMOVE1;
				}
				else{
					p->v = p->vy = ((4 - p->GrpID) * 32 * 3);
					p->a = 0;
					p->State = STG4ROCK_ACCMOVE1;
				}

				break;
			}

			if(p->y > (250+40)*64 || p->y < (-250-40)*64){
				p->x     = (rnd()%(700*64)-350*64);
				p->y     = (250+40)*64;
				p->vy    = ((4 - p->GrpID) * 32 * 3);
				p->v     = 10;
				p->a     = -4;
			}*/
		break;

		case(STG4ROCK_LEAVE):
			if(p->y > (500+40)*64) break;
			else p->y += p->vy;
		break;

		case(STG4ROCK_END):
			if(p->y > (500+40)*64) break;
			else {
				p->y += ((4 - p->GrpID) * 32 * 6);
			}
		break;

		default:
		break;
		}
	}
}


void DrawStg4Rock(void)
{
	constexpr auto sid       = SURFACE_ID::MAPCHIP;
	static PIXEL_LTRB src[3] ={{0, 224, 80, 288},{0, 288, 48, 336},{48, 288, 80, 320}};
	static int dx[3] = {80/2, 48/2, 32/2};
	static int dy[3] = {64/2, 48/2, 32/2};

	int x, y;

	for(const auto& it : Rock) {
		const auto *p = &it;
		x = (p->x + GX_MID)>>6;
		y = (p->y + GY_MID)>>6;
		GrpSurface_Blit(
			{ (x - dx[p->GrpID]), (y - dy[p->GrpID]) }, sid, src[p->GrpID]
		);
	}
}


void SendCmdStg4Rock(uint8_t Cmd, uint8_t Param)
{
	switch(Cmd){
	case(STG4ROCK_LEAVE): {
		for(auto& it : Rock) {
			it.State = STG4ROCK_LEAVE;
		}
	} break;

	case(STG4ROCK_END): {
		for(auto& it : Rock) {
			it.State = STG4ROCK_END;
		}
	} break;

	case(STG4ROCK_ACCMOVE1): {
		for(auto& it : Rock) {
			it.State = STG4ROCK_ACCMOVE1;
			it.a = (it.v / 24); // ((3 - p->GrpID) * 3);
			it.count = 0;
		}
	} break;

	case(STG4ROCK_ACCMOVE2): {
		for(auto& it : Rock) {
			it.State = STG4ROCK_ACCMOVE2;
			it.a = (it.v / 12); // 24; // ((3 - p->GrpID) * 3);
			it.count = 0;
		}
/*		for(auto& it : Rock) {
			it.State = STG4ROCK_ACCMOVE2;
			it.a = -4;
			it.count = 0;
			it.v = 10;
			it.d = Param;
		}
*/
	} break;

	default:
	break;
	}
}



#define S6RASTER_MAX	28
#define S6STAR_MAX		60
#define S3STAR_MAX		180

typedef struct tagStg6Raster {
	int		x, y;		// 表示座標
	char	vy;			//
	uint8_t	type;	// 種類(0-2)
	uint8_t	deg;	// 基準角度
	uint8_t	amp;	// 振幅
} Stg6Raster;

typedef struct tagStg6Star {
	int		x, y;
	int		vy;
} Stg6Star;

Stg6Raster	S6Ras[S6RASTER_MAX];
Stg6Star	S6Star[S3STAR_MAX];		// 兼用モノなのだ


// ６面ラスター初期化 //
void InitStg6Raster()
{
	int		i;

	for(i=0; i<S6RASTER_MAX; i++){
		S6Ras[i].x = rnd()%(640-256) + 128;
		S6Ras[i].y = -rnd()%(480+160);//(480+240)-240;
		S6Ras[i].deg = rnd();
		S6Ras[i].type = i%3;
		S6Ras[i].amp = rnd()%80 + 70;
		S6Ras[i].vy = 2 + rnd()%3;
	}

	for(i=0; i<S6STAR_MAX; i++){
		S6Star[i].x = rnd()%(640-256) + 128;
		S6Star[i].y = rnd()%480;
		S6Star[i].vy = rnd()%20 + 8;
	}
}


// ６面ラスター動作 //
void MoveStg6Raster()
{
	int		i;

	for(i=0; i<S6RASTER_MAX; i++){
		if(i&1) S6Ras[i].deg += 2;
		else    S6Ras[i].deg -= 2;

		S6Ras[i].y += S6Ras[i].vy;

		if(S6Ras[i].y > 480){
			S6Ras[i].x = rnd()%(640-256) + 128;
			S6Ras[i].y = -160;
			S6Ras[i].deg = rnd();
			S6Ras[i].amp = rnd()%80 + 70;
		}
	}

	for(i=0; i<S6STAR_MAX; i++){
		S6Star[i].y += S6Star[i].vy;

		if(S6Star[i].y > 480){
			S6Star[i].x  = rnd()%(640-256) + 128;
			S6Star[i].y -= 480;
			S6Star[i].vy = rnd()%20 + 8;
		}
	}
}


// ６面ラスター描画 //
void DrawStg6Raster()
{
	constexpr auto sid = SURFACE_ID::MAPCHIP;
	static PIXEL_LTRB Target[3] = {
		{608, 272, 640, 352}, {592, 160, 640, 272}, {576, 0, 640, 160},
	};

	PIXEL_LTRB	src;
	int		i, j, h, w;
	int		x1, x2, dx, oy;

	for(i=0; i<S6STAR_MAX; i++){
		src = { 624, 352, (624 + 16), (352 + 16) };
		GrpSurface_Blit({ S6Star[i].x, S6Star[i].y }, sid, src);
	}

	for(const auto& it : S6Ras) {
		x1 = Target[it.type].left;
		x2 = Target[it.type].right;
		oy = Target[it.type].top;
		h = (Target[it.type].bottom - Target[it.type].top);
		w  = (x2 - x1)/2;
		for(j=0; j<h; j+=2){
			dx = sinl((it.deg + j), it.amp);
			src = { x1, (j + oy), x2, (j + 2) };
			GrpSurface_Blit({ (it.x + dx - w), (it.y + j) }, sid, src);
		}
	}
}


// ３面高速星初期化 //
void InitStg3Star()
{
	int		i;

	for(i=0; i<S3STAR_MAX; i++){
		S6Star[i].x = rnd()%(640-256) + 128;
		S6Star[i].y = rnd()%480;
		S6Star[i].vy = rnd()%20 + 8;
	}
}


// ３面高速星動作 //
void MoveStg3Star()
{
	int		i;

	for(i=0; i<S3STAR_MAX; i++){
		S6Star[i].y += S6Star[i].vy;

		if(S6Star[i].y > 480){
			S6Star[i].x  = rnd()%(640-256) + 128;
			S6Star[i].y -= 480;
			S6Star[i].vy = rnd()%20 + 8;
		}
	}
}


// ３面高速星描画 //
void DrawStg3Star()
{
	int		i;

	for(i=0; i<S3STAR_MAX; i++){
		constexpr PIXEL_LTRB src = { (640 - 16), 0, 640, 16 };
		GrpSurface_Blit({ S6Star[i].x, S6Star[i].y }, SURFACE_ID::MAPCHIP, src);
	}
}
