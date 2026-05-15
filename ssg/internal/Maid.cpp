/*                                                                           */
/*   Maid.cpp   メイドさん関連の処理                                         */
/*                                                                           */
/*                                                                           */

#include <printf/printf.h>

#include "ssg/internal/Maid.hpp"
#include "ssg/Gian.h"
#include "ssg/Hook.h"
#include "ssg/Round.h"
#include "ssg/Sound.h"
#include "ssg/internal/Fragment.hpp"
#include "ssg/internal/Laser.hpp"
#include "ssg/internal/MaidTama.hpp"
#include "ssg/internal/PRankCtrl.hpp"
#include "ssg/internal/SEffect.hpp"
#include "hatoyama/logic/cast.h"


// Avoids changes to the original code that accessed the single old `Viv`
// instance. Very ugly, I know, but having a
//
// 	MAID& Viv = *this;
//
// member in `C_VIV` would reload the pointer on every access.
#define Viv (*this)


void C_VIV::Move(INPUT_BITS input, bool in_msg)
{
	int		vx,vy,v;
	constexpr int VivSpeed = (64 * 18);
	char		buf[100];

	// かすり残り時間を減らす //
	if(Viv.evade_c){
		if(Viv.bomb_time && Viv.evade_c>=2) Viv.evade_c -= 2;
		else                                Viv.evade_c -= 1;

		if(Viv.evade_c==0){
			sprintf(buf,"%3d Evade  %7dPts",Viv.evade,Viv.evadesc);
			SEffect.SetString(180, 40, buf);
			score_add(Viv.evadesc);
			Viv.evade   = 0;
			Viv.evadesc = 0;
		}
	}

	// 無敵時間を減らす(ボム中は減らさない) //
	if(Viv.muteki && Viv.bomb_time==0) Viv.muteki--;

	// 得点変化処理 //
	if(Viv.dscore>=100000)		Viv.score+=100000	,Viv.dscore-=100000;
	else if(Viv.dscore>=20000)	Viv.score+=20000	,Viv.dscore-=20000;
	else if(Viv.dscore>=2000)	Viv.score+=2000		,Viv.dscore-=2000;
	else if(Viv.dscore>=200)	Viv.score+=200		,Viv.dscore-=200;
	else if(Viv.dscore>=20)		Viv.score+=20		,Viv.dscore-=20;
	else if(Viv.dscore>=10)		Viv.score+=10		,Viv.dscore-=10;

	// 押しっぱなし減速を有効にするのか //
	if(Round.InputFlags & INPF_Z_SPDDOWN_ENABLE) {
		if(input & KEY_TAMA) {
			if(Viv.ShiftCounter < 8) {
				Viv.ShiftCounter++;
			} else {
				input |= KEY_SHIFT;
			}
		} else {
			Viv.ShiftCounter = 0;
		}
	}

	if(Viv.muteki < MAID_MOVE_DISABLE_TIME){
		vx = vy = 0;
		v = ((input & KEY_SHIFT) ? (VivSpeed / 3) : VivSpeed);
		if(input & KEY_UP) {
			vy -= v;
		}
		if(input & KEY_DOWN) {
			vy += v;
		}
		if(input & KEY_LEFT) {
			vx -= v;
		}
		if(input & KEY_RIGHT) {
			vx += v;
		}

		if(vx && vy){
			Viv.x += (vx/6);
			Viv.y += (vy/6);
		}
		else{
			Viv.x += (vx>>2);
			Viv.y += (vy>>2);
		}

		if(Viv.y<SY_MIN)		Viv.y = SY_MIN;
		else if(Viv.y>SY_MAX)	Viv.y = SY_MAX;

		if(Viv.x<SX_MIN)		Viv.x = SX_MIN;
		else if(Viv.x>SX_MAX)	Viv.x = SX_MAX;
	}
	else{
		vx = 0;
		vy = -(64+32);
		Viv.y += vy;
	}

	if(vx>0)		Viv.GrpID = 2;
	else if(vx<0)	Viv.GrpID = 0;
	else            Viv.GrpID = 1;

	Viv.opx = Viv.x;
	Viv.opy = Viv.y;

	// オプションの処理 //
	if(Viv.vx<0) Viv.vx+=64;
	if(Viv.vx>0) Viv.vx-=64;
	if(Viv.vy<0) Viv.vy+=64;
	if(Viv.vy>0) Viv.vy-=64;

	if(vx<0&&Viv.vx< 6*64)	Viv.vx+=2*64;
	if(vx>0&&Viv.vx>-6*64)	Viv.vx-=2*64;
	if(vy<0&&Viv.vy< 10*64)	Viv.vy+=2*64;
	if(vy>0&&Viv.vy>-10*64)	Viv.vy-=2*64;

	Viv.opx = Viv.x + Viv.vx;
	Viv.opy = Viv.y + Viv.vy + 64*6;

	// 弾＆ボムのセット //
	MaidTama.Set(input, in_msg);

	if(Viv.bomb_time){
		Tama.Clear();
		Laser.Clear();
		// if((Viv.bomb_time % 60) == 0) {
		// 	Fragment.Set(Viv.x, Viv.y, FRG_FATCIRCLE);
		// }
	}

	Viv.BuzzSound = false;
}

// 初期化 //
void C_VIV::Set(uint8_t stage_first)
{
	Viv.score     = 0;
	Viv.dscore    = 0;
	Viv.exp       = Round.Exp;
	Viv.exp2      = 0;
	Viv.bomb      = Round.BombStock;
	Viv.left      = Round.PlayerStock;
	Viv.credit    = ((stage_first == STAGE_EXTRA) ? 0 : 4 /* 5 */);

	Viv.evade_c   = Viv.evade = 0;
	Viv.evadesc   = 0;
	Viv.evade_sum = 0;
	Viv.weapon    = Round.Weapon;
	Viv.GrpID     = 1;

	Viv.GameOverTimer = 0;
}

// 次のステージの準備 //
void C_VIV::NextStage(void)
{
	Viv.x         = Viv.opx = SX_START;
	Viv.y         = Viv.opx = SY_START;
	Viv.vx        = 0;
	Viv.vy        = 0;

	Viv.toge_ex   = 0;
	Viv.toge_time = 0;
	Viv.lay_time  = 0;
	Viv.lay_grp   = 0;

	Viv.muteki    = VIVDEAD_VAL;
	Viv.bomb_time = 0;
	Viv.ShiftCounter = 0;

	Viv.BuzzSound = false;
}

void C_VIV::Dead(void)
{
	int		i;

	Fragment.Set(Viv.x, Viv.y, FRG_FATCIRCLE);

	for(i=0; i<50; i++)
		Fragment.Set(Viv.x, Viv.y, FRG_HEART);

	Hooks.Snd_SEPlay(SOUND_ID_DEAD, SND_X_MID, false, &Hooks);

	// 座標系セット //
	Viv.x         = Viv.opx = SX_START;
	Viv.y         = Viv.opx = SY_START;
	Viv.vx        = 0;
	Viv.vy        = 0;

	// かすり系リセット //
//	Viv.evade_c = 0;
//	Viv.evade   = 0;
//	Viv.evadesc = 0;

	Viv.lay_time  = 0;
	Viv.lay_grp   = 0;

	Viv.bomb = Round.BombStock;
	Viv.muteki = VIVDEAD_VAL;

	PlayRank.Add(-2560);

	if(Viv.left){
		// 残機の残っている場合 //
		Viv.left -= 1;
	}
	else{
		// 残機の残っていない場合 //
		Viv.GameOverTimer = 120;
		Viv.score    += Viv.dscore;			// 得点吐き出し

		// かすり系リセット //
		Viv.evade_c = 0;
		Viv.evade   = 0;
		Viv.evadesc = 0;

		SEffect.SetGameOver();
		Hooks.GameOver(&Hooks); // ゲームオーバーへと
	}

	Tama.Clear();
	Laser.Clear();
}

bool C_VIV::Continue(void)
{
	if(Viv.GameOverTimer >= 1) {
		assert(!"Game Over animation must completely run before continuing");
		return false;
	}
	if(Viv.credit == 0) {
		return false;
	}
	Viv.GameOverTimer = 0;
	Viv.evade_sum = 0;
	Viv.left = Round.PlayerStock;
	Viv.score = ((Viv.score % 10) + 1);
	Viv.credit -= 1;

	SEffect.Init();
	return true;
}

// かすりゲージを上昇させる(エフェクトはメイド中心) //
void C_VIV::evade_add(uint8_t n)
{
	evade_addEx(Viv.x,Viv.y,n);
}

// 指定座標からエフェクト発生＆ゲージ上昇 //
void C_VIV::evade_addEx(int x, int y, uint8_t n)
{
	int		i;

	PlayRank.Add(n << 2);

	if(n){
		if(Viv.BuzzSound == false) {
			Hooks.Snd_SEPlay(SOUND_ID_BUZZ, x, false, &Hooks);
			Viv.BuzzSound = true;
		}
		Fragment.Set(x, y, FRG_EVADE);
		Fragment.Set(x, y, FRG_EVADE);
		Fragment.Set(x, y, FRG_EVADE);
	}

	for(i=0;i<n;i++){
		if(Viv.evade==999){
			Viv.evade_c = 1;
			return;
		}
		Viv.evade   += 1;
		Viv.evade_sum += 1;
		Viv.evadesc += Viv.evade*20;
	}

	if(Viv.evade)
		Viv.evade_c  = EVADETIME_MAX;
}

// スコアを上昇させる //
void C_VIV::score_add(int sc)
{
	Viv.dscore += sc;
}

void C_VIV::PowerUp(uint8_t damage)
{
	// ダメージの分だけ加算する //
	Viv.exp2 += damage;

	// Viv.exp(8bit+1bit) ooo oooooo //
	switch((Cast::up<uint16_t>(Viv.exp) + 1) >> 5) {
	case 0:	if(Viv.exp2 >   2) { Viv.exp++, Viv.exp2 = 0; } return;
	case 1:	if(Viv.exp2 >  10) { Viv.exp++, Viv.exp2 = 0; } return;
	case 2:	if(Viv.exp2 >  30) { Viv.exp++, Viv.exp2 = 0; } return;
	case 3:	if(Viv.exp2 >  80) { Viv.exp++, Viv.exp2 = 0; } return;
	case 4:	if(Viv.exp2 > 120) { Viv.exp++, Viv.exp2 = 0; } return;
	case 5:	if(Viv.exp2 > 140) { Viv.exp++, Viv.exp2 = 0; } return;
	case 6:	if(Viv.exp2 > 160) { Viv.exp++, Viv.exp2 = 0; } return;
	case 7:	if(Viv.exp2 > 180) { Viv.exp++, Viv.exp2 = 0; } return;
	case 8:	return; // フルパワーアップ時
	}
}

#undef Viv

uint8_t GetLaserDeg(const MAID& Viv)
{
	return (((120 - Viv.bomb_time) * 3) / 2);
}

// MSVC's static analyzer suggests to make the functions below `constexpr`,
// which won't work because they are used in other translation units and this
// is not a header.
#pragma warning(suppress: 26497) // f.4
uint8_t GetLeftOrRightLaserDeg(uint8_t LaserDeg, int i)
{
	return ((LaserDeg < 58)
		? ((LaserDeg * 3) + ((i * (64 - LaserDeg)) / 2))
		: ((58 * 3) + ((i * (64 - (std::min)(62, int{ LaserDeg }))) / 2))
	);
}

uint8_t GetRightLaserDeg(uint8_t LaserDeg, int i)
{
	return (64 + 48 - GetLeftOrRightLaserDeg(LaserDeg, i));
}

uint8_t GetLeftLaserDeg(uint8_t LaserDeg, int i)
{
	return (64 - 48 + GetLeftOrRightLaserDeg(LaserDeg, i));
}
