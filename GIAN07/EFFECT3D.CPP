/*                                                                           */
/*   Effect3D.cpp   ３Ｄエフェクトの処理                                     */
/*                                                                           */
/*                                                                           */

#include "EFFECT3D.H"
#include "GIAN.H"
#include "ssg/internal/RNG.hpp"
#include "hatoyama/logic/ut_math.h"


C_EFFECT3D Effect3D;

void Transform3D(Point3D *p, uint8_t dx, uint8_t dy, uint8_t dz)
{
	Point3D temp;

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



void C_EFFECT3D::Init3DCube(void)
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


void C_EFFECT3D::Move3DCube(void)
{
	int				i;
	int				l,d2;
	auto& d = this->cube_d;
	auto& dx = this->cube_dx;
	auto& dy = this->cube_dy;
	constexpr uint16_t dz = 0;

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

void C_EFFECT3D::InitFakeECL(void)
{
	WFLine.d = 0;
	WFLine.ox = 640 * 64 / 2;
	WFLine.oy = 480 * 64 / 2;
	WFLine.w = 30;

	for(auto& it : FakeECLStr) {
		ResetFakeECLStr(it, RNG);
	}
}

void C_EFFECT3D::MoveFakeECL(void)
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

static WORLD_COORD Stg4RockRandomX(C_RNG& RNG)
{
	return ((RNG.next() % PixelToWorld(500)) - PixelToWorld(250));
}

static WORLD_COORD Stg4RockRandomY(C_RNG& RNG)
{
	return PixelToWorld(-250 - 40 - (RNG.next() % 250));
}

void C_EFFECT3D::InitStg4Rock(void)
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


void C_EFFECT3D::MoveStg4Rock(void)
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


void C_EFFECT3D::SendCmdStg4Rock(uint8_t Cmd, uint8_t Param)
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



// ６面ラスター初期化 //
void C_EFFECT3D::InitStg6Raster()
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
void C_EFFECT3D::MoveStg6Raster()
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


// ３面高速星初期化 //
void C_EFFECT3D::InitStg3Star()
{
	int		i;

	for(i=0; i<S3STAR_MAX; i++){
		S6Star[i].x = ((RNG.next() % X_SIZ) + X_MIN);
		S6Star[i].y = ((RNG.next() % Y_SIZ) + Y_MIN);
		S6Star[i].vy = ((RNG.next() % 20) + 8);
	}
}


// ３面高速星動作 //
void C_EFFECT3D::MoveStg3Star()
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


void C_EFFECT3D::Set(EFFECT3D_TYPE effect)
{
	switch(effect) {
	case EFFECT3D_TYPE::NONE:        break;
	case EFFECT3D_TYPE::S3_STAR:     this->InitStg3Star(); break;
	case EFFECT3D_TYPE::S4_ROCK:     this->InitStg4Rock(); break;
	case EFFECT3D_TYPE::S6_RASTER:   this->InitStg6Raster(); break;
	case EFFECT3D_TYPE::S6_FAKE_ECL: this->InitFakeECL(); break;
	case EFFECT3D_TYPE::S6_CUBE:     this->Init3DCube(); break;
	}
	Active = effect;
}

void C_EFFECT3D::Move(void)
{
	switch(Active) {
	case EFFECT3D_TYPE::NONE:        break;
	case EFFECT3D_TYPE::S3_STAR:     this->MoveStg3Star(); break;
	case EFFECT3D_TYPE::S4_ROCK:     this->MoveStg4Rock(); break;
	case EFFECT3D_TYPE::S6_RASTER:   this->MoveStg6Raster(); break;
	case EFFECT3D_TYPE::S6_FAKE_ECL: this->MoveFakeECL(); break;
	case EFFECT3D_TYPE::S6_CUBE:     this->Move3DCube(); break;
	}
}
