/*                                                                           */
/*   LLaser.cpp   長いレーザーの処理                                         */
/*                                                                           */
/*                                                                           */

#include "LLASER.H"
#include "ENEMY.H"
#include "MAID.H"
#include "hatoyama/engine/snd.h"
#include "hatoyama/logic/ut_math.h"


//// レーザー変数２ ////
LLASER_DATA	LLaser[LLASER_MAX];
LLASER_CMD	LLaserCmd;


//// ローカル関数 ////
static void _LLaserPointSet(LLASER_DATA *lp);
static void _LLaserHitCheck(const LLASER_DATA *lp);
static void _LLaserXYSet(int id);			// レーザーの座標をセットする


bool LLaserSet(uint8_t id)
{
	// この部分で空いているレーザーのサーチを行う             //
	// もし、レーザーが見つからなければ、FALSE をリターンする //
	// つまり、その場合は、参照カウントを増加させない         //
	auto lp = std::ranges::find_if(LLaser, [](const auto& lp) {
		return (lp.flag == LLF_DISABLE);
	});
	if(lp == std::end(LLaser)) {
		return false;
	}

	lp->dx = LLaserCmd.dx;
	lp->dy = LLaserCmd.dy;
	lp->e  = LLaserCmd.e;

	lp->EnemyID = id;

	lp->x = lp->e->x + lp->dx;
	lp->y = lp->e->y + lp->dy;

	lp->v = LLaserCmd.v;

	lp->c = LLaserCmd.c;

	lp->lx = 0;
	lp->ly = 0;
	lp->wx = 0;
	lp->wy = 0;

	lp->w    = 0;
	lp->wmax = LLaserCmd.w;

	lp->d = LLaserCmd.d;

	if(LLaserCmd.type == LLS_LONGZ){
		lp->d += atan8(Viv.x-lp->x, Viv.y-lp->y);
		lp->type = LLS_LONG;
	}
	else
		lp->type = LLaserCmd.type;

	lp->infx = cosl(lp->d,800);
	lp->infy = sinl(lp->d,800);

	lp->count = 0;

	_LLaserPointSet(lp);				// p[4] をセット

	lp->flag = LLF_LINE;

	// Snd_SEPlay(1, LaserCmd.x);

	return true;
}

extern void LLaserOpen(const ENEMY_DATA *e, uint8_t id)
{
	for(auto& it : LLaser) {
		auto *lp = &it;
		if((lp->e==e) && (lp->EnemyID==id || id==ECLCST_LLASERALL) && lp->flag!=LLF_DISABLE){
			lp->flag = LLF_OPEN;
			Snd_SEPlay(2, lp->x, true);
		}
	}
}

extern void LLaserClose(const ENEMY_DATA *e, uint8_t id)
{
	if(id==ECLCST_LLASERALL){
		LLaserForceClose(e);
		return;
	}

	for(auto& it : LLaser) {
		auto *lp = &it;
		if((lp->e==e) && (lp->EnemyID==id)){
			lp->flag = LLF_CLOSE;
			Snd_SEStop(2);
		}
	}
}

extern void LLaserLine(const ENEMY_DATA *e, uint8_t id)
{
	for(auto& it : LLaser) {
		auto *lp = &it;
		if((lp->e==e) && (lp->EnemyID==id ||id==ECLCST_LLASERALL)){
			lp->flag = LLF_CLOSEL;
			Snd_SEStop(2);
		}
	}
}

static void _LLaserXYSet(int id)
{
	// 注意！！この関数のid は旧式のid の意味を持つことに注意 //

	LLaser[id].x = LLaser[id].e->x + LLaser[id].dx;
	LLaser[id].y = LLaser[id].e->y + LLaser[id].dy;

	_LLaserPointSet(LLaser+id);			// p[4] をセット
}

extern void LLaserDegA(const ENEMY_DATA *e, uint8_t d, uint8_t id)
{
	for(auto& it : LLaser) {
		auto *lp = &it;
		if((lp->e==e) && (lp->EnemyID==id ||id==ECLCST_LLASERALL)){
			lp->d = d;

			lp->lx = cosl(lp->d,lp->w>>6);
			lp->ly = sinl(lp->d,lp->w>>6);

			lp->wx = -(lp->ly);			// lx,ly を６４だけ回転
			lp->wy =   lp->lx ;			//

			lp->infx = cosl(lp->d,800);
			lp->infy = sinl(lp->d,800);

			_LLaserPointSet(lp);				// p[4] をセット
		}
	}
}

extern void LLaserDegR(const ENEMY_DATA *e, char d, uint8_t id)
{
	for(auto& it : LLaser) {
		auto *lp = &it;
		if((lp->e==e) && (lp->EnemyID==id ||id==ECLCST_LLASERALL)){
			lp->d += d;

			lp->lx = cosl(lp->d,lp->w>>6);
			lp->ly = sinl(lp->d,lp->w>>6);

			lp->wx = -(lp->ly);			// lx,ly を６４だけ回転
			lp->wy =   lp->lx ;			//

			lp->infx = cosl(lp->d,800);
			lp->infy = sinl(lp->d,800);

			_LLaserPointSet(lp);				// p[4] をセット
		}
	}
}

// 敵に関連づけられたレーザーを強制クローズ(Level2...) //
extern void LLaserForceClose(const ENEMY_DATA *e)
{
	for(auto& it : LLaser) {
		auto *lp = &it;

		// (参考) すでにクローズ状態であっても LLaserClose() は問題を発生させない //
		if(lp->e==e){
			lp->flag = LLF_CLOSE;
			Snd_SEStop(2);
			//LLaserClose(i);
		}
	}
}

extern void LLaserMove(void)
{
	int				i;
	LLASER_DATA		*lp;

	for(i=0,lp=LLaser;i<LLASER_MAX;i++,lp++){

		// 角度セットモードで、敵の角度と現在の角度が異なっていたら再度セット //
		if(lp->type==LLS_SETDEG && lp->e && lp->d!=lp->e->d){
			lp->d = lp->e->d;

			lp->lx = cosl(lp->d,lp->w>>6);
			lp->ly = sinl(lp->d,lp->w>>6);

			lp->wx = -(lp->ly);			// lx,ly を６４だけ回転
			lp->wy =   lp->lx ;			//

			lp->infx = cosl(lp->d,800);
			lp->infy = sinl(lp->d,800);

			_LLaserPointSet(lp);				// p[4] をセット
		}

		switch(lp->flag){
			// 太くなる場合 //
			case(LLF_OPEN):
				_LLaserXYSet(i);
				lp->w += lp->v;

				if((lp->w)>=(lp->wmax)){
					lp->w    = lp->wmax;
					lp->flag = LLF_NORM;
				}

				lp->lx = cosl(lp->d,lp->w>>6);
				lp->ly = sinl(lp->d,lp->w>>6);
				lp->wx = -(lp->ly);
				lp->wy =   lp->lx;

				_LLaserPointSet(lp);	// p[4] をセット
				_LLaserHitCheck(lp);
			break;

			// 細くなる場合 //
			case(LLF_CLOSE):case(LLF_CLOSEL):
				_LLaserXYSet(i);
				lp->w -= lp->v;

				if((lp->w)<=0){
					lp->w = 0;
					if(lp->flag == LLF_CLOSE){
						lp->flag = LLF_DISABLE;
						lp->e    = nullptr;
					}
					else                      lp->flag = LLF_LINE;
				}

				lp->lx = cosl(lp->d,lp->w>>6);
				lp->ly = sinl(lp->d,lp->w>>6);
				lp->wx = -(lp->ly);
				lp->wy =   lp->lx;

				_LLaserPointSet(lp);	// p[4] をセット
				//_LLaserHitCheck(lp);
			break;

			// 直線状態 //
			case(LLF_LINE):
				_LLaserXYSet(i);
				// この部分にレーザー溜めエフェクトを仕掛ける //
				//Fragment.Set(lp->x, lp->y, FRG_LASER);
			break;

			// ノーマル //
			case(LLF_NORM):
				_LLaserXYSet(i);
				_LLaserHitCheck(lp);
			break;

			case(LLF_DISABLE):
			break;
		}
	}
}

extern void LLaserClear(void)
{
	// 存在するレーザー全てを閉じる //
	for(auto& it : LLaser) {
		if(it.flag != LLF_DISABLE) {
			it.flag = LLF_CLOSE;
		}
	}

	Snd_SEStop(2);
}

extern void LLaserSetup(void)
{
	for(auto& it : LLaser) {
		//memset(LLaser+i,0,sizeof(LLASER_DATA));
		it.flag = LLF_DISABLE;
		it.e    = nullptr;
	}

	Snd_SEStop(2);
}

static void _LLaserPointSet(LLASER_DATA *lp)
{
	auto *pp = lp->p;

	pp[1].x = pp[0].x = (lp->x>>6) + lp->wx + lp->lx;
	pp[1].y = pp[0].y = (lp->y>>6) + lp->wy + lp->ly;

	pp[2].x = pp[3].x = (lp->x>>6) - lp->wx + lp->lx;
	pp[2].y = pp[3].y = (lp->y>>6) - lp->wy + lp->ly;

	pp[1].x += lp->infx;
	pp[1].y += lp->infy;

	pp[2].x += lp->infx;
	pp[2].y += lp->infy;
}

static void _LLaserHitCheck(const LLASER_DATA *lp)
{
//	long tx,ty,w1,w2,length;

	int		tx,ty;
	int		length,width;

	if(Viv.muteki) return;

	tx = Viv.x - lp->x;
	ty = Viv.y - lp->y;

	length = cosl(lp->d,tx) + sinl(lp->d,ty);
	width  = abs(-sinl(lp->d,tx) + cosl(lp->d,ty));

/*
	// 計算上の注意 : 座標の計算にはx64をつかう //
	// sinm(),cosm()を使っているので/256補正が必要となる //
	tx = ((lp->x)-(Viv.x));	ty = ((lp->y)-(Viv.y));
	length = -((cosm(lp->d)*tx+sinm(lp->d)*ty)>>8);
	tx <<= 8;	ty <<= 8;

	if(cosm(lp->d)==0)
		w1 = abs((length*cosm(lp->d)+tx)/(-sinm(lp->d)));
	else if(sinm(lp->d)==0)
		w1 = abs((length*sinm(lp->d)+ty)/( cosm(lp->d)));
	else{
		w2 = abs((length*cosm(lp->d)+tx)/(-sinm(lp->d)));
		w1 = abs((length*sinm(lp->d)+ty)/( cosm(lp->d)));
		w1 = (w1+w2)/2;	// 精度アップ
	}
*/
	if(length>0 && width<=(lp->w+64*15)){
		evade_add(LLASER_EVADE);
	}
	if(length>0 && width<=(lp->w)){
		MaidDead();
	}
}
