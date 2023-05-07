/*
 *   BombEfc.cpp   : 爆発系エフェクト
 *
 */

#include "BOMBEFC.H"
#include "LOADER.H"
#include "DirectXUTYs/DD_UTY.H"
#include "DirectXUTYs/UT_MATH.H"


BombEfcCtrl		BombEfc[EXBOMB_MAX];

// 秘密の関数 //
void _ExBombSTDInit(BombEfcCtrl *p);
void _ExBombSTDDraw(BombEfcCtrl *p);
void _ExBombSTDMove(BombEfcCtrl *p);



// 爆発系エフェクトの初期化 //
void ExBombEfcInit(void)
{
	int				i;

	for(i=0; i<EXBOMB_MAX; i++){
		BombEfc[i].bIsUsed = FALSE;
	}
}


// 爆発系エフェクトをセットする //
void ExBombEfcSet(int x, int y, uint8_t type)
{
	int				i;
	BombEfcCtrl		*p;

	for(i=0; i<EXBOMB_MAX; i++){
		if(FALSE == BombEfc[i].bIsUsed) break;
	}
	// 空いているオブジェクトが存在しない //
	if(i == EXBOMB_MAX) return;

	// ターゲット捕捉 //
	p = BombEfc + i;

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
	p->bIsUsed = TRUE;
}


// 爆発系エフェクトを描画する
void ExBombEfcDraw(void)
{
	int				i;

	for(i=0; i<EXBOMB_MAX; i++){
		if(!BombEfc[i].bIsUsed) continue;
		switch(BombEfc[i].type){
			case EXBOMB_STD:	_ExBombSTDDraw(BombEfc+i);	break;

			default:
			return;
		}
	}
}


// 爆発系エフェクトを動作させる
void ExBombEfcMove(void)
{
	int				i;

	for(i=0; i<EXBOMB_MAX; i++){
		if(!BombEfc[i].bIsUsed) continue;
		BombEfc[i].count++;
		switch(BombEfc[i].type){
			case EXBOMB_STD:
				_ExBombSTDMove(BombEfc+i);
				if(BombEfc[i].count > 64*3+32) BombEfc[i].bIsUsed = FALSE;
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
	int			i, x, y, dx;
	SpObj		*target;

	// Graphic 48 * 48 //
	target = p->Obj;
	for(i=0; i<EXBOMB_OBJMAX; i++, target++){
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
	int			i, j, x, y, v, rv;
	DWORD		t;
	BYTE		d, rd;
	SpObj		*target;

	x = p->x;
	y = p->y;
	t = p->count;
	v = sinl(t/2-64, 200*64)+(200*64);

	target = p->Obj;
	for(i=j=0; i<EXBOMB_OBJMAX; i++, target++){
		if(target->d > 7*2){
			if(t > 64*3) continue;
			rd = rnd();
			rv = rnd()%256+128;
			target->vx = cosl(rd, rv);
			target->vy = sinl(rd, rv);

			d = t*2 + ((j%8)<<5);// + rnd()%8 - 4;
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
