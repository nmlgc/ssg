/*
 *   BombEfc.cpp   : 爆発系エフェクト
 *
 */

#include "BOMBEFC.H"
#include "LOADER.H"
#include "DirectXUTYs/DD_UTY.H"
#include "game/ut_math.h"


BombEfcCtrl		BombEfc[EXBOMB_MAX];

// 秘密の関数 //
void _ExBombSTDInit(BombEfcCtrl *p);
void _ExBombSTDDraw(BombEfcCtrl *p);
void _ExBombSTDMove(BombEfcCtrl *p);



// 爆発系エフェクトの初期化 //
void ExBombEfcInit(void)
{
	for(auto& it : BombEfc) {
		it.bIsUsed = false;
	}
}


// 爆発系エフェクトをセットする //
void ExBombEfcSet(int x, int y, uint8_t type)
{
	auto p = std::ranges::find_if(BombEfc, [](const auto& p) {
		return !p.bIsUsed;
	});

	// 空いているオブジェクトが存在しない //
	if(p == std::end(BombEfc)) {
		return;
	}

	// ターゲット捕捉 //
	p->x     = x;
	p->y     = y;
	p->type  = type;
	p->count = 0;

	switch(type){
		case EXBOMB_STD:	_ExBombSTDInit(p);	break;

		default:
		return;
	}

	// ここまできたら、無事にエフェクトがセットされた事になる //
	p->bIsUsed = true;
}


// 爆発系エフェクトを描画する
void ExBombEfcDraw(void)
{
	for(auto& it : BombEfc) {
		if(!it.bIsUsed) {
			continue;
		}
		switch(it.type) {
		case EXBOMB_STD:
			_ExBombSTDDraw(&it);
			break;

		default:
			return;
		}
	}
}


// 爆発系エフェクトを動作させる
void ExBombEfcMove(void)
{
	for(auto& it : BombEfc) {
		if(!it.bIsUsed) {
			continue;
		}
		it.count++;
		switch(it.type) {
		case EXBOMB_STD:
			_ExBombSTDMove(&it);
			if(it.count > ((64 * 3) + 32)) {
				it.bIsUsed = false;
			}
			break;

			default:
			return;
		}
	}
}


void _ExBombSTDInit(BombEfcCtrl *p)
{
	int			i, x, y;
	SpObj		*target;

	x = p->x;
	y = p->y;

	target = p->Obj;
	for(i=0; i<EXBOMB_OBJMAX; i++, target++){
		target->x = x;
		target->y = y;
		target->d = i % (7*2);
	}
}


void _ExBombSTDDraw(BombEfcCtrl *p)
{
	int x, y, dx;

	// Graphic 48 * 48 //
	for(const auto& it : p->Obj) {
		const auto *target = &it;
		if(target->d <= 7*2){
			x = (target->x >> 6) - 24;
			y = (target->y >> 6) - 24;
			dx = (target->d>>1) * 48;
			PIXEL_LTRB src = PIXEL_LTWH{ dx, 296, 48, 48 };
			GrpBlt(&src, x, y, GrTama);
		}
	}
}


void _ExBombSTDMove(BombEfcCtrl *p)
{
	int j = 0;
	int x, y, v, rv;

	x = p->x;
	y = p->y;
	const auto t = p->count;
	v = sinl(t/2-64, 200*64)+(200*64);

	for(auto& it : p->Obj) {
		auto *target = &it;
		if(target->d > 7*2){
			if(t > 64*3) continue;
			const uint8_t rd = rnd();
			rv = rnd()%256+128;
			target->vx = cosl(rd, rv);
			target->vy = sinl(rd, rv);

			const uint8_t d = ((t * 2) + ((j % 8) << 5)); // + (rnd() % 8) - 4;
			//v = sinl(degx,64*90)+64*90;

			rv = v - rnd()%(v>>2);
			target->x = cosl(d,rv) + x;
			target->y = sinl(d,rv) + y;
			target->d = 0;

			j++;
		}
		else{
			target->d = target->d + 1;
			target->x += target->vx;
			target->y += target->vy;
			target->vx -= (target->vx>>3);
			target->vy -= (target->vy>>3);
		}
	}
}
