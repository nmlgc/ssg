/*                                                                           */
/*   Effect3D.cpp   ３Ｄエフェクトの処理                                     */
/*                                                                           */
/*                                                                           */

#include "EFFECT3D.H"
#include "GIAN.H"
#include "ssg/internal/RNG.hpp"
#include "hatoyama/engine/graphics_backend.h"
#include "hatoyama/logic/cast.h"
#include "hatoyama/logic/ut_math.h"


Cube3D		Cube[CUBE_MAX];
Star2D		Star[STAR_MAX];
Rock3D		Rock[ROCK_MAX];

WFLine2D		WFLine;
FakeECLString	FakeECLStr[FAKE_ECLSTR_MAX];

template <size_t N> consteval auto TransformLineList(
	PIXEL_POINT center, std::array<PIXEL_POINT, N>&& in
)
{
	std::array<WORLD_POINT, N> ret;
	for(size_t i = 0; auto& p : ret) {
		p = (in[i++] - center);
	}
	return ret;
}

constinit auto PList_W = TransformLineList<11>({ 192, 39 }, {{
	{  0, 15 }, { 15, 66 }, { 32, 47 }, { 48, 66 },
	{ 63, 14 }, { 52, 11 }, { 42, 38 }, { 32, 26 },
	{ 21, 38 }, { 11, 10 }, {  0, 15 },
}});

constinit auto PList_A1 = TransformLineList<8>({ 192, 39 }, {{
	{  96, 12 }, {  66, 61 }, {  75, 67 }, { 83, 56 },
	{ 107, 56 }, { 115, 67 }, { 125, 61 }, { 96, 12 },
}});

constinit auto PList_A2 = TransformLineList<4>({ 192, 39 }, {{
	{ 96, 34 }, { 90, 44 }, { 101, 44 }, { 96, 34 },
}});

constinit auto PList_R = TransformLineList<15-1>({ 192, 39 }, {{
	{ 132, 14 }, { 132, 64 }, { 145, 64 }, { 145, 27 },
	{ 164, 27 },

	//{ 150, 41 }, { 150, 43 },
	{ 150, 42 },

	{ 171, 66 },
	{ 173, 66 }, { 181, 57 }, { 167, 43 }, { 180, 29 },
	{ 180, 27 }, { 170, 14 }, { 132, 14 },
}});

constinit auto PList_N1 = TransformLineList<9>({ 192, 39 }, {{
	{ 189, 12 }, { 189, 64 }, { 201, 64 }, { 201, 40 },
	{ 239, 66 }, { 239, 14 }, { 227, 14 }, { 227, 38 },
	{ 189, 12 },
}});

constinit auto PList_N2 = TransformLineList<9>({ (192 - (296 - 215)), 39 }, {{
	{ 189, 12 }, { 189, 64 }, { 201, 64 }, { 201, 40 },
	{ 239, 66 }, { 239, 14 }, { 227, 14 }, { 227, 38 },
	{ 189, 12 },
}});

constinit auto PList_I = TransformLineList<5>({ 192, 39 }, {{
	{ 248, 14 }, { 248, 64 }, { 262, 64 }, { 262, 14 },
	{ 248, 14 },
}});

constinit auto PList_G = TransformLineList<17>({ 192, 39 }, {{
	{ 354, 11 }, { 328, 22 }, { 328, 57 }, { 354, 68 },
	{ 380, 59 }, { 380, 34 }, { 355, 34 }, { 354, 45 },
	{ 367, 46 }, { 367, 51 }, { 355, 55 }, { 342, 50 },
	{ 342, 29 }, { 354, 24 }, { 372, 30 }, { 377, 19 },
	{ 354, 11 },
}});

LineList3D Warning[8] = {
	{ PList_W },
	{ PList_A1 },
	{ PList_A2 },
	{ PList_R },
	{ PList_N1 },
	{ PList_I },
	{ PList_N2 },
	{ PList_G },
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



void DrawWarning(void)
{
	static const PIXEL_LTRB src = { 0, (152 + 16), 384, (232 + 16) };
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
		Cube[i].d.dx = RNG.next();
		Cube[i].d.dy = RNG.next();
		Cube[i].d.dz = RNG.next();
		Cube[i].p.x = cosl(i*256/CUBE_MAX, 200*64);
		Cube[i].p.y = sinl(i*256/CUBE_MAX, 200*64);
		Cube[i].p.z = 0;
	}

	for(auto& it : Star) {
		it.x = ((RNG.next() % X_SIZ) + X_MIN);
		it.y = -(RNG.next() % Y_SIZ);
		it.vy = ((RNG.next() % 10) + 10);
	}
}


void Draw3DCube(void)
{
	for(const auto& it : Star) {
		static const PIXEL_LTWH rc = { 136, 272, 16, 24 };
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
			it.x = ((RNG.next() % X_SIZ) + X_MIN);
			it.y = 0;
			it.vy = ((RNG.next() % 10) + 10);
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


static void ResetFakeECLStr(FakeECLString& it, C_RNG& RNG)
{
	const uint8_t d = (RNG.next() % 128);
	const int v = ((RNG.next() % PixelToWorld(5)) + PixelToWorld(5));

	it.SrcX = (((RNG.next() % 7) * 9) * 8);
	it.SrcY = ((RNG.next() % 16) * 16);
	it.x    = PixelToWorld(28 + (RNG.next() % 484));
	it.y    = -PixelToWorld(RNG.next() % 640);
	it.vx   = 0; //cosl(d, v);
	it.vy   = v; //sinl(d, v);
}

void InitEffectFakeECL(void)
{
	WFLine.d = 0;
	WFLine.ox = 640 * 64 / 2;
	WFLine.oy = 480 * 64 / 2;
	WFLine.w = 30;

	for(auto& it : FakeECLStr) {
		ResetFakeECLStr(it, RNG);
	}
}

void MoveEffectFakeECL(void)
{
	WFLine.ox = (WFLine.ox+1)%64;
	WFLine.oy = (WFLine.oy+62)%64;

	for(auto& it : FakeECLStr) {
		it.x += it.vx;
		it.y += it.vy;

		if(it.y >= (480 * 64)) {
			ResetFakeECLStr(it, RNG);
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

static WORLD_COORD Stg4RockRandomX(C_RNG& RNG)
{
	return ((RNG.next() % PixelToWorld(500)) - PixelToWorld(250));
}

static WORLD_COORD Stg4RockRandomY(C_RNG& RNG)
{
	return PixelToWorld(-250 - 40 - (RNG.next() % 250));
}

void InitStg4Rock(void)
{
	int		i;
	int		id = 2;
	int		y  = 0;
	constexpr int dy = (500 * (64 / 4));
	constexpr int dy2 = (dy / 2);

	for(i=0; i<ROCK_MAX; i++){
		y = (((i % 4) * dy) + (RNG.next() % dy2));
			//((380*64/16) * (i%(ROCK_MAX/16+1))) + rnd()%(380*64/16);

		Rock[i].x = Stg4RockRandomX(RNG);	//
		Rock[i].y = -250*64-y;						// 上の方なのだ
		Rock[i].z = Stg4RockRandomX(RNG);	//

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
				p->x     = Stg4RockRandomX(RNG);
				p->y     = Stg4RockRandomY(RNG);
				p->vy    = ((4 - p->GrpID) * 16);
				p->v     = p->vy;
			}
		break;

		case(STG4ROCK_ACCMOVE1):
			p->v += p->a;
			p->vy = p->v;
			p->y += p->vy;

			if(p->y > (250+40)*64){
				p->x     = Stg4RockRandomX(RNG);
				p->y     = Stg4RockRandomY(RNG);
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
					p->x     = Stg4RockRandomX(RNG);
					p->y     = Stg4RockRandomY(RNG);
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
				p->x     = Stg4RockRandomX(RNG);
				p->y     = (PixelToWorld(250 + 40) + (RNG.next() % 250));
				p->vy    = (-(4 - p->GrpID) * 32 * 3);
				p->v     = p->vy;
				p->a     = 0;
			}
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
	static const PIXEL_LTRB src[3] = {
		{  0, 224, 80, 288 },
		{  0, 288, 48, 336 },
		{ 48, 288, 80, 320 },
	};
	static const int dx[3] = { (80 / 2), (48 / 2), (32 / 2) };
	static const int dy[3] = { (64 / 2), (48 / 2), (32 / 2) };

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



Stg6Raster	S6Ras[S6RASTER_MAX];
Stg6Star	S6Star[S3STAR_MAX];		// 兼用モノなのだ


// ６面ラスター初期化 //
void InitStg6Raster()
{
	int		i;

	for(i=0; i<S6RASTER_MAX; i++){
		S6Ras[i].x = ((RNG.next() % X_SIZ) + X_MIN);
		S6Ras[i].y = (-RNG.next() % (480 + 160)); //(480+240)-240;
		S6Ras[i].deg = RNG.next();
		S6Ras[i].type = i%3;
		S6Ras[i].amp = ((RNG.next() % 80) + 70);
		S6Ras[i].vy = (2 + (RNG.next() % 3));
	}

	for(i=0; i<S6STAR_MAX; i++){
		S6Star[i].x = ((RNG.next() % X_SIZ) + X_MIN);
		S6Star[i].y = ((RNG.next() % Y_SIZ) + Y_MIN);
		S6Star[i].vy = ((RNG.next() % 20) + 8);
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
			S6Ras[i].x = ((RNG.next() % X_SIZ) + X_MIN);
			S6Ras[i].y = -160;
			S6Ras[i].deg = RNG.next();
			S6Ras[i].amp = ((RNG.next() % 80) + 70);
		}
	}

	for(i=0; i<S6STAR_MAX; i++){
		S6Star[i].y += S6Star[i].vy;

		if(S6Star[i].y > 480){
			S6Star[i].x = ((RNG.next() % X_SIZ) + X_MIN);
			S6Star[i].y -= 480;
			S6Star[i].vy = ((RNG.next() % 20) + 8);
		}
	}
}


// ６面ラスター描画 //
void DrawStg6Raster()
{
	constexpr auto sid = SURFACE_ID::MAPCHIP;
	static const PIXEL_LTRB Target[3] = {
		{ 608, 272, 640, 352 },
		{ 592, 160, 640, 272 },
		{ 576,   0, 640, 160 },
	};

	int		i, j, h, w;
	int		x1, x2, dx, oy;

	for(i=0; i<S6STAR_MAX; i++){
		static const PIXEL_LTRB src = { 624, 352, (624 + 16), (352 + 16) };
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
			const PIXEL_LTRB src = { x1, (j + oy), x2, (j + 2) };
			GrpSurface_Blit({ (it.x + dx - w), (it.y + j) }, sid, src);
		}
	}
}


// ３面高速星初期化 //
void InitStg3Star()
{
	int		i;

	for(i=0; i<S3STAR_MAX; i++){
		S6Star[i].x = ((RNG.next() % X_SIZ) + X_MIN);
		S6Star[i].y = ((RNG.next() % Y_SIZ) + Y_MIN);
		S6Star[i].vy = ((RNG.next() % 20) + 8);
	}
}


// ３面高速星動作 //
void MoveStg3Star()
{
	int		i;

	for(i=0; i<S3STAR_MAX; i++){
		S6Star[i].y += S6Star[i].vy;

		if(S6Star[i].y > 480){
			S6Star[i].x = ((RNG.next() % X_SIZ) + X_MIN);
			S6Star[i].y -= 480;
			S6Star[i].vy = ((RNG.next() % 20) + 8);
		}
	}
}


// ３面高速星描画 //
void DrawStg3Star()
{
	int		i;

	for(i=0; i<S3STAR_MAX; i++){
		static const PIXEL_LTRB src = { (640 - 16), 0, 640, 16 };
		GrpSurface_Blit({ S6Star[i].x, S6Star[i].y }, SURFACE_ID::MAPCHIP, src);
	}
}
