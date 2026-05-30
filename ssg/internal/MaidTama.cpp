/*                                                                           */
/*   MaidTama.cpp   メイドさんなショットの処理                               */
/*                                                                           */
/*                                                                           */

#include "ssg/internal/MaidTama.hpp"
#include "ssg/Gian.h"
#include "ssg/Hook.h"
#include "ssg/Sound.h"
#include "ssg/internal/Enemy.hpp"
#include "ssg/internal/Fragment.hpp"
#include "ssg/internal/Maid.hpp"
#include "ssg/internal/PRankCtrl.hpp"
#include "hatoyama/logic/cast.h"
#include "hatoyama/logic/ut_math.h"


constexpr uint8_t TogeDamage[(4 * 2) + 2] = {
	// MainWeapon		// SubWeapon
	TDM_WIDE_MAIN,		TDM_WIDE_SUB,		// TYPE_A(WIDE)
	TDM_HOMING_MAIN,	TDM_HOMING_SUB,		// TYPE_B(HOMING)
	TDM_LASER_MAIN,		TDM_LASER_SUB,		// TYPE_C
	1,					1					// ホーミングボム用
};

#define WIDE_BOMB_TIME			(60*4)
#define HOMING_BOMB_TIME		(60*3)
#define LASER_BOMB_TIME			(60*2)
#define CACTUS_BOMB_TIME		(0)

static constexpr uint8_t MaidBombTime[4] = {
	WIDE_BOMB_TIME, HOMING_BOMB_TIME, LASER_BOMB_TIME, CACTUS_BOMB_TIME
};


#define MAID_TAMA_START		18	//12
#define MAID_MAIN_SHOT		6	//4
#define MAID_SUB_SHOT		9	//6



//// 弾コマンド用マクロ ////
void C_MAIDTAMA::TamaSTDForm(uint8_t c)
{
	TamaCmd.cmd    = TC_WAY;
	TamaCmd.option = TOP_NONE;
	TamaCmd.type   = T_NORM;
	TamaCmd.c      = c;
}

void C_MAIDTAMA::TamaSetHoming(uint8_t c)
{
	TamaCmd.cmd    = TC_WAY;
	TamaCmd.option = TOP_NONE;
	TamaCmd.type   = T_SBHOMING;
	TamaCmd.c      = c;
	TamaCmd.rep    = 64;
	TamaCmd.vd     = 5;
}

void C_MAIDTAMA::TamaSetDeg(uint8_t d, uint8_t dw)
{
	TamaCmd.d  = d;
	TamaCmd.dw = dw;
}

void C_MAIDTAMA::TamaSetNum(uint8_t n, uint8_t ns)
{
	TamaCmd.n  = n;
	TamaCmd.ns = ns;
}

void C_MAIDTAMA::TamaSetSpd(uint8_t v, char a)
{
	TamaCmd.v = v;
	TamaCmd.a = a;
}

void C_MAIDTAMA::TamaSetXY(int x, int y)
{
	TamaCmd.x = x;
	TamaCmd.y = y;
}



// たま発射！！ //
void C_MAIDTAMA::Set(INPUT_BITS input, bool in_msg)
{
	// この関数では、前回の発射状態 (Viv_St) を参照して、発射可能であるならば //
	// 発射し、そうでなければ、単にリターンする。                             //
	// なお、弾のセットには TAMA.cpp 内の関数と互換のものを使用する           //

	if(
		(input & KEY_TAMA) &&
		(Viv.toge_time == 0) &&
		(Viv.muteki < MAID_MOVE_DISABLE_TIME)
	) {
		Viv.toge_time = MAID_TAMA_START;
	}

	// ボムの発動条件を満たしていれば、発動! //
	if(
		(input & KEY_BOMB) &&
		(Viv.bomb_time == 0) &&
		// (Viv.muteki == 0) &&
		Viv.bomb &&
		(in_msg == false)
	) {
		//if(Viv.weapon == 0) EnterBombPalette();

		Viv.bomb_time = MaidBombTime[Viv.weapon&3];		// 装備ごとに変更せよ
		Viv.muteki    = BOMBMUTEKI_VAL;
		Viv.bomb--;
		PlayRank.Add(-500); // 難易度ダウン
	}

	if(Viv.bomb_time){
		Viv.bomb_time--;
		std::invoke(MaidBombFunc[Viv.weapon], *this);

		//if(Viv.bomb_time == 0 && Viv.weapon == 0)
		//	LeaveBombPalette();
	}

	if(Viv.toge_time){
		std::invoke(MaidTamaFunc[Viv.weapon&3][(Viv.exp+1)>>5], *this);
		Viv.toge_time--;
	}

	// レーザーを装備している場合 //
	if(Viv.weapon == 2  &&  Viv.lay_time){
		Viv.lay_time--;
		if(Viv.lay_time<64)				Viv.lay_grp = 0;
		else if(Viv.lay_time<64+50)		Viv.lay_grp = 1;
		else if(Viv.lay_time<64+100)	Viv.lay_grp = 2;
		else if(Viv.lay_time<64+150)	Viv.lay_grp = 3;
		else							Viv.lay_grp = 4;
		//Viv.lay_grp = (Viv.lay_time+63)>>6;
	}
}

// 弾移動＆ヒットチェック //
void C_MAIDTAMA::Move(void)
{
	// この関数では、TAMA.cpp の敵弾の移動処理を使用する。もちろん、         //
	// 当たり判定については、敵に対してのものとする事！！                    //
	// 当たり判定は、この弾の座標を与えることで ENEMY.cpp 内の関数が判別して //
	// 敵に当たっているかをチェックするものとする。                          //

	int				i;

	for(i=0;i<MaidTamaNow;i++){
		auto* t = &MaidTama[MaidTamaInd[i]];
		if(t->c == TID_HOMING_BOMB_B){
			Enemy.Damage(t->x, t->y, TogeDamage[t->c]);
			t->count++;
			if(t->count>=19) t->flag = TF_DELETE;
			continue;
		}
		if(t->effect==TE_NONE){
			Tama.Tmove(t);
			Tama.Omove(t);
			t->count++;
			if(((t->flag&TF_CLIP)==0)&&((t->x)<GX_MIN||(t->x)>GX_MAX||(t->y)<GY_MIN||(t->y)>GY_MAX))
				t->flag = TF_DELETE;

			if(Enemy.Damage(t->x, t->y, TogeDamage[t->c])) {
				if(t->c == TID_HOMING_BOMB_A){
					TamaSTDForm(TID_HOMING_BOMB_B);
					TamaCmd.type = T_SBHBOMB;
					TamaSetXY(t->x,t->y);
					TamaSetDeg(-64,16);
					TamaSetSpd(10,0);
					TamaSetNum(1,0);
					MTamaSet();
				}
				t->flag = TF_DELETE;
				Fragment.Set(t->x, t->y, FRG_HIT);
			}
		}
		else
			Tama.Emove(t);
	}
	Indsort(MaidTamaInd, MaidTamaNow, MaidTama, [](const TAMA_DATA& t) {
		return (t.flag & TF_DELETE);
	});


	// レーザーの当たり判定 //
	if(Viv.weapon == 2  &&  Viv.lay_grp){
		//x = (Viv.opx>>6)+4 -8 + SBOPT_DX;
		//y = (Viv.opy>>6)-20;
		Enemy.Damage2(
			(Viv.opx + (SBOPT_DX << 6)), Viv.opy, ((Viv.lay_grp / 3) + 1)
		);
		Enemy.Damage2(
			(Viv.opx - (SBOPT_DX << 6)), Viv.opy, ((Viv.lay_grp / 3) + 1)
		);
	}
}

// 弾ハッシュテーブル初期化 //
void C_MAIDTAMA::IndSet(void)
{
	// この配列を初期化することで全ての弾を初期化する事になる //
	for(MAIDTAMA_DATA_IND i = 0; i < MAIDTAMA_MAX; i++) {
		MaidTamaInd[i] = i;
		//memset(MaidTama+i,0,sizeof(TAMA_DATA));
	}

	// 現在の個数を０初期化するのを忘れずに //
	MaidTamaNow = 0;
}

void C_MAIDTAMA::MTamaSet(void)
{
	for(decltype(TamaCmd.n) i = 0; i < TamaCmd.n; i++) {
		if(MaidTamaNow+1 >= MAIDTAMA_MAX) return;			// セットできない場合

		auto* t = &MaidTama[MaidTamaInd[MaidTamaNow++]];	// 弾ポインタをセット

		Tama.SetSingle(*t, i, SPEEDM(TamaCmd.v), 0);
	}
}

#define IsMainShot(t)	((t)==MAID_MAIN_SHOT||(t)==MAID_MAIN_SHOT*2||(t)==MAID_MAIN_SHOT*3)
#define IsSubShot(t)	( ((t)==0 || (t)==MAID_SUB_SHOT) && Viv.bomb_time==0)

void C_MAIDTAMA::MLaserSet(uint16_t time)
{
	if(Viv.bomb_time || Viv.muteki>MAID_MOVE_DISABLE_TIME){
		Viv.lay_time = 0;
		Viv.lay_grp  = 0;
		return;
	}

	if(Viv.lay_time == 0){
		Viv.lay_time = time;
		Hooks.Snd_SEPlay(SOUND_ID_SBLASER, Viv.x, false, &Hooks);
	}
}

// ショットＴＹＰＥ－Ａ //
void C_MAIDTAMA::SetT_A0(void)
{
	if(IsMainShot(Viv.toge_time)){
		// 中央にショット単発のみ //
		TamaSTDForm(TID_WIDE_MAIN);
		TamaSetXY(Viv.x,Viv.y);
		TamaSetDeg(-64,0);
		TamaSetSpd(54,0);
		TamaSetNum(1,0);
		MTamaSet();
	}
}

void C_MAIDTAMA::SetT_A1(void)
{
	char dd;

	if(IsSubShot(Viv.toge_time)){
		// オプションのショット(右) //
		TamaSTDForm(TID_WIDE_SUB);
		TamaSetXY(Viv.opx+SBOPT_DX*64,Viv.opy);
		TamaSetDeg(-64+5,0);
		TamaSetSpd(54,0);
		TamaSetNum(1,0);
		MTamaSet();

		// オプションのショット(左) //
		TamaSetXY(Viv.opx-SBOPT_DX*64,Viv.opy);
		TamaSetDeg(-64-5,0);
		MTamaSet();
	}

	if(IsMainShot(Viv.toge_time)){
		// 軽く振り分けるメインショット //
		Viv.toge_ex += 32;
		dd = Cast::down<int8_t>(sinl(Viv.toge_ex, 6));
		TamaSTDForm(TID_WIDE_MAIN);
		TamaSetXY(Viv.x,Viv.y);
		TamaSetDeg(-64+dd,0);
		TamaSetSpd(54,0);
		TamaSetNum(1,0);
		MTamaSet();
	}
}

void C_MAIDTAMA::SetT_A2(void)
{
	char dd;


	if(IsMainShot(Viv.toge_time)){
		// 中央にショット２連 //
		Viv.toge_ex += 32;
		dd = Cast::down<int8_t>(sinl(Viv.toge_ex, 6));

		TamaSTDForm(TID_WIDE_MAIN);
		TamaSetXY(Viv.x-(6*64),Viv.y);
		TamaSetDeg(-64+dd,0);
		TamaSetSpd(54,0);
		TamaSetNum(1,0);
		MTamaSet();
		TamaCmd.x += (12*64);
		MTamaSet();
	}

	if(IsSubShot(Viv.toge_time)){
		// オプションのショット(右) //
		TamaSTDForm(TID_WIDE_SUB);
		TamaSetXY(Viv.opx+SBOPT_DX*64,Viv.opy);
		TamaSetSpd(54,0);
		TamaSetNum(1,0);
		TamaSetDeg(-64+5,0);
		MTamaSet();

		// オプションのショット(左) //
		TamaSetXY(Viv.opx-SBOPT_DX*64,Viv.opy);
		TamaSetDeg(-64-5,0);
		MTamaSet();
	}
}

void C_MAIDTAMA::SetT_A3(void)
{
	char dd;

	if(IsMainShot(Viv.toge_time)){
		// 中央にショット３ＷＡＹ //
		Viv.toge_ex+=32;
		dd = Cast::down<int8_t>(sinl(Viv.toge_ex, 6));
		TamaSTDForm(TID_WIDE_MAIN);
		TamaSetXY(Viv.x,Viv.y);
		TamaSetDeg(-64+dd,4);
		TamaSetSpd(54,0);
		TamaSetNum(3,0);
		MTamaSet();
	}

	if(IsSubShot(Viv.toge_time)){
		// オプションのショット(右) //
		TamaSTDForm(TID_WIDE_SUB);
		TamaSetXY(Viv.opx+SBOPT_DX*64,Viv.opy);
		TamaSetDeg(-64+5,0);
		TamaSetSpd(54,0);
		TamaSetNum(1,0);
		MTamaSet();

		// オプションのショット(左) //
		TamaSetXY(Viv.opx-SBOPT_DX*64,Viv.opy);
		TamaSetDeg(-64-5,0);
		MTamaSet();
	}
}

void C_MAIDTAMA::SetT_A4(void)
{
	char dd;

	if(IsMainShot(Viv.toge_time)){
		// 中央にショット３ＷＡＹ //
		Viv.toge_ex+=32;
		dd = Cast::down<int8_t>(sinl(Viv.toge_ex, 6));
		TamaSTDForm(TID_WIDE_MAIN);
		TamaSetXY(Viv.x,Viv.y);
		TamaSetDeg(-64+dd,4);
		TamaSetSpd(54,0);
		TamaSetNum(3,0);
		MTamaSet();
	}

	if(IsSubShot(Viv.toge_time)){
		// オプションのショット(右) //
		TamaSTDForm(TID_WIDE_SUB);
		TamaSetXY(Viv.opx+SBOPT_DX*64,Viv.opy);
		TamaSetDeg(-64+8,7);//(-64+5,7);
		TamaSetSpd(54,0);
		TamaSetNum(2,0);
		MTamaSet();

		// オプションのショット(左) //
		TamaSetXY(Viv.opx-SBOPT_DX*64,Viv.opy);
		TamaSetDeg(-64-8,7);//(-64-5,7);
		MTamaSet();
	}
}

void C_MAIDTAMA::SetT_A5(void)
{
	SetT_A4();
}

void C_MAIDTAMA::SetT_A6(void)
{
	char dd;

	if(IsMainShot(Viv.toge_time)){
		// 中央にショット４ＷＡＹ //
		Viv.toge_ex+=32;
		dd = Cast::down<int8_t>(sinl(Viv.toge_ex, 6));
		TamaSTDForm(TID_WIDE_MAIN);
		TamaSetXY(Viv.x,Viv.y);
		TamaSetDeg(-64+dd,3);
		TamaSetSpd(54,0);
		TamaSetNum(5,0);
		MTamaSet();
	}

	if(IsSubShot(Viv.toge_time)){
		// オプションのショット(右) //
		TamaSTDForm(TID_WIDE_SUB);
		TamaSetXY(Viv.opx+SBOPT_DX*64,Viv.opy);
		TamaSetDeg(-64+10,8);//-64+6,4);
		TamaSetSpd(54,0);
		TamaSetNum(3,0);
		MTamaSet();

		// オプションのショット(左) //
		TamaSetXY(Viv.opx-SBOPT_DX*64,Viv.opy);
		TamaSetDeg(-64-10,8);//(-64-6,4);
		MTamaSet();
	}
}

void C_MAIDTAMA::SetT_A7(void)
{
	SetT_A6();
}

void C_MAIDTAMA::SetT_A8(void)
{
	char dd;

	if(IsMainShot(Viv.toge_time)){
		// 中央にショット４ＷＡＹ //
		Viv.toge_ex+=32;
		dd = Cast::down<int8_t>(sinl(Viv.toge_ex, 6));
		TamaSTDForm(TID_WIDE_MAIN);
		TamaSetXY(Viv.x,Viv.y);
		TamaSetDeg(-64+dd,3);
		TamaSetSpd(54,0);
		TamaSetNum(5,0);
		MTamaSet();
	}

	if(IsSubShot(Viv.toge_time)){
		// オプションのショット(右) //
		TamaSTDForm(TID_WIDE_SUB);
		TamaSetXY(Viv.opx+SBOPT_DX*64,Viv.opy);
		TamaSetDeg(-64+12,8);//(-64+7,4);
		TamaSetSpd(54,0);
		TamaSetNum(4,0);
		MTamaSet();

		// オプションのショット(左) //
		TamaSetXY(Viv.opx-SBOPT_DX*64,Viv.opy);
		TamaSetDeg(-64-12,8);//(-64-7,4);
		MTamaSet();
	}
}

// ショットＴＹＰＥ－Ｂ //
void C_MAIDTAMA::SetT_B0(void)
{
	char dd;

	if(IsMainShot(Viv.toge_time)){
		// 軽く振り分けるメインショット //
		Viv.toge_ex+=32;
		dd = Cast::down<int8_t>(sinl(Viv.toge_ex, 4));
		TamaSTDForm(TID_HOMING_MAIN);
		TamaSetXY(Viv.x,Viv.y);
		TamaSetDeg(-64+dd,0);
		TamaSetSpd(54,0);
		TamaSetNum(1,0);
		MTamaSet();
	}
	//Viv.toge_time = 3;
}

void C_MAIDTAMA::SetT_B1(void)
{
	if(IsMainShot(Viv.toge_time)){
		// 中央にショット２連 //
		TamaSTDForm(TID_HOMING_MAIN);
		TamaSetXY(Viv.x-(6*64),Viv.y);
		TamaSetDeg(-64,0);
		TamaSetSpd(54,0);
		TamaSetNum(1,0);
		MTamaSet();
		TamaCmd.x += (12*64);
		MTamaSet();
	}
	//Viv.toge_time = 4;
	//if((++Viv.toge_ex)&7) return;

	if(IsSubShot(Viv.toge_time)){
		// ホーミング弾 //
		// オプションのショット(右) //
		TamaSetHoming(TID_HOMING_SUB);
		TamaSetXY(Viv.opx+SBOPT_DX*64,Viv.opy);
		TamaSetSpd(28,4);
		TamaSetDeg(64-5,0);
		TamaSetNum(1,0);
		MTamaSet();

		// オプションのショット(左) //
		TamaSetXY(Viv.opx-SBOPT_DX*64,Viv.opy);
		TamaSetDeg(64+5,0);
		MTamaSet();
	}
}

void C_MAIDTAMA::SetT_B2(void)
{
	if(IsMainShot(Viv.toge_time)){
		// 中央にショット３ＷＡＹ //
		TamaSTDForm(TID_HOMING_MAIN);
		TamaSetXY(Viv.x,Viv.y);
		TamaSetDeg(-64,7);
		TamaSetSpd(54,0);
		TamaSetNum(3,0);
		MTamaSet();
	}

	//Viv.toge_time = 4;
	//if((++Viv.toge_ex)&7) return;

	if(IsSubShot(Viv.toge_time)){
		// ホーミング弾 //
		// オプションのショット(右) //
		TamaSetHoming(TID_HOMING_SUB);
		TamaSetXY(Viv.opx+SBOPT_DX*64,Viv.opy);
		TamaSetSpd(28,4);
		TamaSetDeg(64-5,0);
		TamaSetNum(1,0);
		MTamaSet();

		// オプションのショット(左) //
		TamaSetXY(Viv.opx-SBOPT_DX*64,Viv.opy);
		TamaSetDeg(64+5,0);
		MTamaSet();
	}
}

void C_MAIDTAMA::SetT_B3(void)
{
	SetT_B2();
}

void C_MAIDTAMA::SetT_B4(void)
{
	if(IsMainShot(Viv.toge_time)){
		// 中央にショット５ＷＡＹ //
		TamaSTDForm(TID_HOMING_MAIN);
		TamaSetXY(Viv.x,Viv.y);
		TamaSetDeg(-64,7);
		TamaSetSpd(54,0);
		TamaSetNum(5,0);
		MTamaSet();
	}

	//Viv.toge_time = 4;
	//if((++Viv.toge_ex)&7) return;

	if(IsSubShot(Viv.toge_time)){
		// ホーミング弾 //
		// オプションのショット(右) //
		TamaSetHoming(TID_HOMING_SUB);
		TamaSetXY(Viv.opx+SBOPT_DX*64,Viv.opy);
		TamaSetSpd(28,4);
		TamaSetDeg(64-5,0);
		TamaSetNum(1,0);
		MTamaSet();

		// オプションのショット(左) //
		TamaSetXY(Viv.opx-SBOPT_DX*64,Viv.opy);
		TamaSetDeg(64+5,0);
		MTamaSet();
	}
}

void C_MAIDTAMA::SetT_B5(void)
{
	SetT_B4();
}

void C_MAIDTAMA::SetT_B6(void)
{
	if(IsMainShot(Viv.toge_time)){
		// 中央にショット５ＷＡＹ //
		TamaSTDForm(TID_HOMING_MAIN);
		TamaSetXY(Viv.x,Viv.y);
		TamaSetDeg(-64,7);
		TamaSetSpd(54,0);
		TamaSetNum(5,0);
		MTamaSet();
	}

	//Viv.toge_time = 4;
	//if((++Viv.toge_ex)&3) return;

	if(IsSubShot(Viv.toge_time)){
		// ホーミング弾 //
		// オプションのショット(右) //
		TamaSetHoming(TID_HOMING_SUB);
		TamaSetXY(Viv.opx+SBOPT_DX*64,Viv.opy);
		TamaSetSpd(28,4);
		TamaSetDeg(64-5,0);
		TamaSetNum(1,0);
		MTamaSet();

		// オプションのショット(左) //
		TamaSetXY(Viv.opx-SBOPT_DX*64,Viv.opy);
		TamaSetDeg(64+5,0);
		MTamaSet();
	}
}

void C_MAIDTAMA::SetT_B7(void)
{
	SetT_B6();
}

void C_MAIDTAMA::SetT_B8(void)
{
	if(IsMainShot(Viv.toge_time)){
		// 中央にショット５ＷＡＹ //
		TamaSTDForm(TID_HOMING_MAIN);
		TamaSetXY(Viv.x,Viv.y);
		TamaSetDeg(-64,7);
		TamaSetSpd(54,0);
		TamaSetNum(5,0);
		MTamaSet();
	}

	//Viv.toge_time = 4;
	//if((++Viv.toge_ex)&3) return;

	if(IsSubShot(Viv.toge_time)){
		// ホーミング弾 //
		// オプションのショット(右) //
		TamaSetHoming(TID_HOMING_SUB);
		TamaSetXY(Viv.opx+SBOPT_DX*64,Viv.opy);
		TamaSetSpd(28,4);
		TamaSetDeg(64-22,30);
		TamaSetNum(2,0);
		MTamaSet();

		// オプションのショット(左) //
		TamaSetXY(Viv.opx-SBOPT_DX*64,Viv.opy);
		TamaSetDeg(64+22,30);
		MTamaSet();
	}
}

// ショットＴＹＰＥ－Ｃ //
void C_MAIDTAMA::SetT_C0(void)
{
	if(IsMainShot(Viv.toge_time)){
		// 中央にショット単発のみ //
		TamaSTDForm(TID_LASER_SUB);
		TamaSetXY(Viv.x,Viv.y);
		TamaSetDeg(-64,0);
		TamaSetSpd(54,0);
		TamaSetNum(1,0);
		MTamaSet();
	}
}

void C_MAIDTAMA::SetT_C1(void)
{
	if(IsMainShot(Viv.toge_time)){
		// 中央に２列ショット //
		TamaSTDForm(TID_LASER_SUB);
		TamaSetXY(Viv.x-(6*64),Viv.y);
		TamaSetDeg(-64,0);
		TamaSetSpd(54,0);
		TamaSetNum(1,0);
		MTamaSet();
		TamaCmd.x += (12*64);
		MTamaSet();
	}

	MLaserSet(64+50);
}

void C_MAIDTAMA::SetT_C2(void)
{
	SetT_C1();
}

void C_MAIDTAMA::SetT_C3(void)
{
	if(IsMainShot(Viv.toge_time)){
		// 中央にショット３ＷＡＹ //
		TamaSTDForm(TID_LASER_SUB);
		TamaSetXY(Viv.x,Viv.y);
		TamaSetDeg(-64,6);
		TamaSetSpd(54,0);
		TamaSetNum(3,0);
		MTamaSet();
	}

	MLaserSet(64+100);
}

void C_MAIDTAMA::SetT_C4(void)
{
	SetT_C3();
}

void C_MAIDTAMA::SetT_C5(void)
{
	if(IsMainShot(Viv.toge_time)){
		// 中央にショット４ＷＡＹ(中央は２列で) //
		TamaSTDForm(TID_LASER_SUB);
		TamaSetSpd(54,0);

		TamaSetDeg(-64-5,10);
		TamaSetXY(Viv.x-(6*64),Viv.y);
		TamaSetNum(2,0);
		MTamaSet();

		TamaSetDeg(-64+5,10);
		TamaCmd.x += (12*64);
		MTamaSet();
	}

	MLaserSet(64+150);
}

void C_MAIDTAMA::SetT_C6(void)
{
	SetT_C5();
}

void C_MAIDTAMA::SetT_C7(void)
{
	SetT_C5();
}

void C_MAIDTAMA::SetT_C8(void)
{
	if(IsMainShot(Viv.toge_time)){
		// 中央にショット５ＷＡＹ //
		TamaSTDForm(TID_LASER_SUB);
		TamaSetXY(Viv.x,Viv.y);
		TamaSetDeg(-64,6);
		TamaSetSpd(54,0);
		TamaSetNum(5,0);
		MTamaSet();
	}

	MLaserSet(64+200);
}

// ショットＴＹＰＥ－Ｄ //
void C_MAIDTAMA::SetT_D0(void)
{
}

void C_MAIDTAMA::SetT_D1(void)
{
}

void C_MAIDTAMA::SetT_D2(void)
{
}

void C_MAIDTAMA::SetT_D3(void)
{
}

void C_MAIDTAMA::SetT_D4(void)
{
}

void C_MAIDTAMA::SetT_D5(void)
{
}

void C_MAIDTAMA::SetT_D6(void)
{
}

void C_MAIDTAMA::SetT_D7(void)
{
}

void C_MAIDTAMA::SetT_D8(void)
{
}



void C_MAIDTAMA::SetWideBomb(void)
{
	int		dx,dy,l;

	if(Viv.bomb_time > WIDE_BOMB_TIME-30) return;

	const uint8_t d  = Cast::down<uint8_t>(Viv.bomb_time * 3u);
	l  = (WIDE_BOMB_TIME - Viv.bomb_time)*26;		// 16-32
	dx = GX_MID + 64*70/2 + cosl(d, l<<1);
	dy = GY_MID - 64*90/2 + sinl(d<<1, l);

	Fragment.Set(dx, dy, FRG_STAR1);
	Fragment.Set(dx, dy, FRG_STAR1);
	Fragment.Set(dx, dy, FRG_STAR2);

	Enemy.Damage4(1);
}

void C_MAIDTAMA::SetHomingBomb(void)
{
	if(Viv.bomb_time%30 == 1){
		TamaSetHoming(TID_HOMING_BOMB_A);
		TamaSetXY(Viv.x,Viv.y);
		TamaSetSpd(28,4);
		TamaSetDeg(64,16);
		TamaSetNum(8,1);
		MTamaSet();

		// 欠陥があるので、廃止 //
		//ObjectLockOn(&HomingX, &HomingY, 32*64, 32*64);
	}
}

// こいつは、Set というよりも、 HitCheck 的な役割を果たす //
void C_MAIDTAMA::SetLaserBomb(void)
{
	int			ox,oy;
	int			i;

	const auto LaserDeg = GetLaserDeg(Viv);

	ox = Viv.opx + (SBOPT_DX*64);
	oy = Viv.opy;
	for(i=-3;i<=3;i++){
		const auto d = GetRightLaserDeg(LaserDeg, i);
		Enemy.Damage3(ox, oy, d);
	}

	ox = Viv.opx - (SBOPT_DX*64);
	oy = Viv.opy;
	for(i=-3;i<=3;i++){
		const auto d = GetLeftLaserDeg(LaserDeg, i);
		Enemy.Damage3(ox, oy, d);
	}
}

void C_MAIDTAMA::SetCactusBomb(void)
{
}
