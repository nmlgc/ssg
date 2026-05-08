/*************************************************************************************************/
/*   TAMA.cpp   たまの発射などに関する関数群                                                     */
/*                                                                                               */
/*************************************************************************************************/

#include "TAMA.H"
#include "EFFECT.H"
#include "ENEMY.H"
#include "FRAGMENT.H"
#include "GIAN.H"
#include "ITEM.H"
#include "MAID.H"
#include "PRankCtrl.h"
#include "ssg/Round.h"
#include "ssg/internal/RNG.hpp"
#include "hatoyama/engine/snd.h"
#include "hatoyama/logic/cast.h"
#include "hatoyama/logic/ut_math.h"

//// ホーミング定数 ////
// 敵をホーミングしない場合のダミー値
static constexpr WORLD_COORD HOMING_DUMMY = PixelToWorld(500);


////グローバル変数////
TAMA_CMD		TamaCmd;				// 標準・弾コマンド構造体
std::array<TAMA_DATA, TAMA_MAX>	Tama;	// 弾の格納用構造体
std::array<uint16_t, TAMA_MAX>	Tama1Ind;	// 小型弾の順番を維持するための配列
std::array<uint16_t, TAMA_MAX>	Tama2Ind;	// 特殊弾の順番を維持するための配列
uint16_t	Tama1Now;	// 小型弾の弾数
uint16_t	Tama2Now;	// 特殊弾の弾数
uint16_t	Tama1Max;	// 小型弾の最大数
uint16_t	Tama2Max;	// 特殊弾の最大数
int				TamaSpeed;

WORLD_COORD HomingX; // ホーミング対象のＸ座標
WORLD_COORD HomingY; // ホーミング対象のＹ座標
int HomingFlag;      // 真ならホーミング実行


////ローカルな関数////
static void __TamaSet(void);
static void easy_cmd(TAMA_CMD& TamaCmd);	// 難易度：Ｅａｓｙ
static void hard_cmd(TAMA_CMD& TamaCmd);	// 難易度：Ｈａｒｄ
static void luna_cmd(TAMA_CMD& TamaCmd);	// 難易度：Ｌｕｎａｔｉｃ

int LineCmdNewTamaSpeed(uint16_t i);
int TamaSpeedEx(uint8_t d);


//// かすり用マクロ ////
static void TamaEvadeAdd(TAMA_DATA *t, C_VIV& Viv)
{
	if(t->flag & TF_EVADE) {
		Viv.evade_addEx(t->x, t->y, 0);
	} else {
		t->flag |= TF_EVADE;
		Viv.evade_addEx(t->x, t->y, TAMA_EVADE);
	}
}

void tama_set(void)
{
	int		v;

	// NORMAL の場合は変更しない(ゲーム中に増減する難易度は考案中) //
	// おそらく switch 中に記述する事になるかと... //
	switch(PlayRank.LevelRanked()) {
		case(GAME_EASY):
			easy_cmd(TamaCmd);
		break;

		case(GAME_HARD):
			hard_cmd(TamaCmd);
		break;

		case(GAME_LUNATIC):
			luna_cmd(TamaCmd);
		break;
	}

	//数値は単純に　(speed /2) *rank/32 + speed/2
	v = SPEEDM(TamaCmd.v);		// 速度の基本値をセットする(GIAN.H)
	if((TamaCmd.type & 0x0f) == T_NORM)
		TamaSpeed = ((((v >> 1) * (PlayRank.Rank())) >> (5 + 8)) + (v >> 1));
	else
		TamaSpeed = v;

	__TamaSet();
}

void tama_setEX(void)
{
	TamaSpeed = SPEEDM(TamaCmd.v);

	__TamaSet();
}

// 弾をセットする(ライン状に発射)
void tama_setLine(void)
{
	// uint32_t temp;
	uint16_t *indnow, *indmax, *indp; // 上に同じ


	TamaSpeed = SPEEDM(TamaCmd.v);


	// "アクセスする領域" をセットする(小型弾 or 特殊弾)        //
	if((TamaCmd.c & 0xf0) == TAMA_SMALL)
		indnow=&Tama1Now,indmax=&Tama1Max,indp=&Tama1Ind[Tama1Now];
	else
		indnow=&Tama2Now,indmax=&Tama2Max,indp=&Tama2Ind[Tama2Now];

	// セットする弾数(連射を考慮に入れる)
	const uint16_t setmax = (
		TamaCmd.n * ((TamaCmd.cmd & TAMA_REN) ? TamaCmd.ns : 1)
	);

	// その他パラメータのセット //
	TamaCmd.cmd = (TamaCmd.cmd & 0xf0) | TC_WAY;

	for(const auto i : std::views::iota(0u, setmax)) {
		if( (*indnow)+1 >= (*indmax) ) return;	// セットできない場合

		*indnow = *indnow + 1;					// 弾数をインクリメント
		auto* t = &Tama[indp[i]];	// 弾ポインタをセット

		t->x = t->tx = TamaCmd.x;				// X座標のセット
		t->y = t->ty = TamaCmd.y;				// Y座標のセット

		t->a = TamaCmd.a;						// 注意：サイズは char

		//temp = random_ref;
		t->d   = tama_dir(i);					// 弾の発射角度
		//Debug(temp,31);

		t->d16 = (t->d<<8);						// 角速度のある運動で使用

		//temp = random_ref;
		t->v = t->v0 = LineCmdNewTamaSpeed(i);	// 初速度のセット
		//Debug(temp,30);

		t->vx = cosl(t->d,t->v);				// 速度のＸ成分セット
		t->vy = sinl(t->d,t->v);				// 速度のＹ成分セット

		t->vd     = TamaCmd.vd;					// 角速度もしくはホーミング率
		t->c      = TamaCmd.c;					// 弾の色＆形状
		t->rep    = TamaCmd.rep;				// 繰り返し回数
		t->type   = TamaCmd.type;				// 弾の種類
		t->option = TamaCmd.option;				// 弾の属性(バイブ、反射等)
		t->effect = TamaCmd.cmd & 0xf0;			// 弾のエフェクト
		t->count  = 0;							// カウンタの初期化
		t->flag   = tama_flag();				// フラグの初期化
	}
}

// エキストラボス専用弾幕(角度が広くなると、遅くなる) //
void tama_setExtra01(void)
{
	// uint32_t temp;
	uint16_t *indnow, *indmax, *indp; // 上に同じ


	TamaSpeed = SPEEDM(TamaCmd.v);


	// "アクセスする領域" をセットする(小型弾 or 特殊弾)        //
	if((TamaCmd.c & 0xf0) == TAMA_SMALL)
		indnow=&Tama1Now,indmax=&Tama1Max,indp=&Tama1Ind[Tama1Now];
	else
		indnow=&Tama2Now,indmax=&Tama2Max,indp=&Tama2Ind[Tama2Now];

	// セットする弾数(連射を考慮に入れる)
	const uint16_t setmax = (
		TamaCmd.n * ((TamaCmd.cmd & TAMA_REN) ? TamaCmd.ns : 1)
	);

	// その他パラメータのセット //
	//TamaCmd.cmd = (TamaCmd.cmd & 0xf0);

	for(const auto i : std::views::iota(0u, setmax)) {
		if( (*indnow)+1 >= (*indmax) ) return;	// セットできない場合

		*indnow = *indnow + 1;					// 弾数をインクリメント
		auto* t = &Tama[indp[i]];	// 弾ポインタをセット

		t->x = t->tx = TamaCmd.x;				// X座標のセット
		t->y = t->ty = TamaCmd.y;				// Y座標のセット

		t->a = TamaCmd.a;						// 注意：サイズは char

		t->d   = tama_dir(i);					// 弾の発射角度
		t->d16 = (t->d<<8);						// 角速度のある運動で使用

		t->v = t->v0 = TamaSpeedEx(t->d);		// 初速度のセット

		t->vx = cosl(t->d,t->v);				// 速度のＸ成分セット
		t->vy = sinl(t->d,t->v);				// 速度のＹ成分セット

		t->vd     = TamaCmd.vd;					// 角速度もしくはホーミング率
		t->c      = TamaCmd.c;					// 弾の色＆形状
		t->rep    = TamaCmd.rep;				// 繰り返し回数
		t->type   = TamaCmd.type;				// 弾の種類
		t->option = TamaCmd.option;				// 弾の属性(バイブ、反射等)
		t->effect = TamaCmd.cmd & 0xf0;			// 弾のエフェクト
		t->count  = 0;							// カウンタの初期化
		t->flag   = tama_flag();				// フラグの初期化
	}
}

int TamaSpeedEx(uint8_t d)
{
	int		temp = 0;
	int		delta;

	switch(TamaCmd.v & 0xc0){
	case(TAMASP_RND1): temp = ((RNG.next() % 16) -  8); break;
	case(TAMASP_RND2): temp = ((RNG.next() % 32) - 16); break;
	case(TAMASP_RND3): temp = ((RNG.next() % 64) - 32); break;
	}

	// d と TamaCmd.d の値の離れ具合により、速度を変化させる //
	delta = TamaCmd.d - d;
	if(delta >  128) delta -= 256;
	if(delta < -128) delta += 256;

	return TamaSpeed - (TamaSpeed * abs(delta)) / 23 + temp;
}

static void __TamaSet(void)
{
	// uint32_t temp;
	uint16_t *indnow, *indmax, *indp; // 上に同じ


	// "アクセスする領域" をセットする(小型弾 or 特殊弾)        //
	if((TamaCmd.c & 0xf0) == TAMA_SMALL)
		indnow=&Tama1Now,indmax=&Tama1Max,indp=&Tama1Ind[Tama1Now];
	else
		indnow=&Tama2Now,indmax=&Tama2Max,indp=&Tama2Ind[Tama2Now];

	// セットする弾数(連射を考慮に入れる)
	const uint16_t setmax = (
		TamaCmd.n * ((TamaCmd.cmd & TAMA_REN) ? TamaCmd.ns : 1)
	);

	for(const auto i : std::views::iota(0u, setmax)) {
		if( (*indnow)+1 >= (*indmax) ) return;	// セットできない場合

		*indnow = *indnow + 1;					// 弾数をインクリメント
		auto* t = &Tama[indp[i]];

		TamaSetSingle(*t, i, TamaSpeed, (TamaCmd.cmd & 0xf0));
	}
}

void tama_move(void)
{
	// ヒットチェック後にサボテンの生死判定をしているのは、死んでいる時間 //
	// よりも生きている時間のほうが長いからなのですが...                  //

	// 小型弾の処理 //
	for(const auto i : std::views::iota(0u, Tama1Now)) {
		auto* t = &Tama[Tama1Ind[i]];
		if(t->effect==TE_NONE){
			tamaTmove(t);
			tamaOmove(t);
			if(((t->flag&TF_CLIP)==0)&&
				((t->x)<GX_MIN-4*64||(t->x)>GX_MAX+4*64||(t->y)<GY_MIN-4*64||(t->y)>GY_MAX+4*64))
				t->flag = TF_DELETE;
			t->count++;
			if(Viv.muteki) continue;
			if(HITCHK(t->x,Viv.x,TAMA_EVX_SMALL) && HITCHK(t->y,Viv.y,TAMA_EVY_SMALL)){
				TamaEvadeAdd(t, Viv);
			}
			if(HITCHK(t->x,Viv.x,TAMA_HITX) && HITCHK(t->y,Viv.y,TAMA_HITY)){
				t->flag = TF_DELETE;
				Viv.Dead();
			}
		}
		else{
			tamaEmove(t);
			t->count++;
		}
	}
	Indsort(Tama1Ind, Tama1Now, Tama);

	// 大型弾＆特殊弾の処理 //
	for(const auto i : std::views::iota(0u, Tama2Now)) {
		auto* t = &Tama[Tama2Ind[i]];
		if(t->effect==TE_NONE){
			tamaTmove(t);
			tamaOmove(t);
			if(((t->flag&TF_CLIP)==0)&&
				((t->x)<GX_MIN-8*64||(t->x)>GX_MAX+8*64||(t->y)<GY_MIN-8*64||(t->y)>GY_MAX+8*64))
				t->flag = TF_DELETE;
			t->count++;
			if(Viv.muteki) continue;
			if(HITCHK(t->x,Viv.x,TAMA_EVX_LARGE) && HITCHK(t->y,Viv.y,TAMA_EVY_LARGE)){
				TamaEvadeAdd(t, Viv);
			}
			if(HITCHK(t->x,Viv.x,TAMA_HITX) && HITCHK(t->y,Viv.y,TAMA_HITY)){
				t->flag = TF_DELETE;
				Viv.Dead();
			}
		}
		else{
			tamaEmove(t);
			t->count++;
		}
	}
	Indsort(Tama2Ind, Tama2Now, Tama);
}


void tama_clear(void)
{
	for(const auto i : std::views::iota(0u, Tama1Now)) {
		auto& t = Tama[Tama1Ind[i]];
		if(t.effect != TE_DELETE){
			t.effect = TE_DELETE;
			t.count  = 0;
			//t.c      = 0x25;
			t.d      = 0;
		}
	}

	for(const auto i : std::views::iota(0u, Tama2Now)) {
		auto& t = Tama[Tama2Ind[i]];
		if(t.effect != TE_DELETE){
			t.effect = TE_DELETE;
			t.count  = 0;
			//t.c      = 0x25;
			t.d      = 0;
		}
	}
}

// 弾を得点化する(Ret : 得点)
uint32_t tama2score(void)
{
	uint32_t sum = 0;
	uint32_t Score;

	Score = TAMA1_POINT + Viv.evade * 100;
	for(const auto i : std::views::iota(0u, Tama1Now)) {
		auto* t = &Tama[Tama1Ind[i]];
		if(t->effect!=TE_DELETE){
			StringEffect2(t->x-64*4,t->y-64*4,Score);
			sum += Score;
			t->flag   = TF_DELETE;
			t->count  = 0;
			t->c      = 0x25;
			t->d      = 0;
		}
	}
	Indsort(Tama1Ind, Tama1Now, Tama);

	Score = TAMA2_POINT + Viv.evade * 100;
	for(const auto i : std::views::iota(0u, Tama2Now)) {
		auto* t = &Tama[Tama2Ind[i]];
		if(t->effect!=TE_DELETE){
			StringEffect2(t->x-64*8,t->y-64*8,Score);
			sum += Score;
			t->flag   = TF_DELETE;
			t->count  = 0;
			t->c      = 0x25;
			t->d      = 0;
		}
	}
	Indsort(Tama2Ind, Tama2Now, Tama);

	return sum;
}

// 弾をアイテム化する //
void tama2item(uint8_t n)
{
	// uint32_t sum = 0;
	// uint32_t Score;

//	Score = TAMA1_POINT + Viv.evade * 100;

	if(n == 0){
		tama_clear();
		return;
	}

	for(const auto i : std::views::iota(0u, Tama1Now)) {
		auto* t = &Tama[Tama1Ind[i]];
		if(t->effect!=TE_DELETE){
			t->count  = 0;
			t->d      = 0;

			if((RNG.next() % n) == 0) {
				Item.Set(t->x, t->y, ITEM_SCORE);
				t->flag   = TF_DELETE;
				t->c      = 0x25;
			}
			else{
				t->effect = TE_DELETE;
				t->count  = 0;
				t->d      = 0;
			}
		}
	}
	Indsort(Tama1Ind, Tama1Now, Tama);

//	Score = TAMA2_POINT + Viv.evade * 100;
	for(const auto i : std::views::iota(0u, Tama2Now)) {
		auto* t = &Tama[Tama2Ind[i]];
		if(t->effect!=TE_DELETE){
			t->count  = 0;
			t->d      = 0;

			if((RNG.next() % n) == 0) {
				Item.Set(t->x, t->y, ITEM_SCORE);
				t->flag   = TF_DELETE;
				t->c      = 0x25;
			}
			else{
				t->effect = TE_DELETE;
				t->count  = 0;
				t->d      = 0;
			}
		}
	}
	Indsort(Tama2Ind, Tama2Now, Tama);

//	return sum;
}

void tamaind_set(uint16_t tama1)
{
	int i;

	if(tama1>=TAMA_MAX) tama1 = TAMA_MAX-1;

	// 弾の最大数のセット //
	Tama1Max=tama1;
	Tama2Max=TAMA_MAX-tama1;

	// 弾のインデックス用配列の初期化 //
	for(i=0;i<tama1;i++)			Tama1Ind[i]       = i;
	for(i=tama1;i<TAMA_MAX;i++)		Tama2Ind[i-tama1] = i;

	//memset(Tama,0,sizeof(TAMA_DATA)*TAMA_MAX);

	Tama1Now = Tama2Now = 0;
}

static void easy_cmd(TAMA_CMD& TamaCmd)
{
	switch(TamaCmd.cmd&0x03){
		case(TC_WAY):
			if(TamaCmd.n>=3) TamaCmd.n-=2;	// 奇数・偶数は変化させない
			TamaCmd.dw += (TamaCmd.dw>>2);	// 幅を広げる
		break;

		case(TC_ALL):case(TC_RND):
			TamaCmd.n>>=1;					// 弾数／２
		break;
	}

	if(TamaCmd.ns>=2) TamaCmd.ns--;			// 連射数_減少
}

static void hard_cmd(TAMA_CMD& TamaCmd)
{
	switch(TamaCmd.cmd&0x03){
		case(TC_WAY):
			TamaCmd.n+=2;					// 奇数・偶数は変化させない
			TamaCmd.dw -= (TamaCmd.dw>>3);	// 幅を狭める
		break;

		case(TC_ALL):
			TamaCmd.n += (( (TamaCmd.n>>2)>6 ) ? 6 : (TamaCmd.n>>2) );
		break;

		case(TC_RND):
			TamaCmd.n += (TamaCmd.n>>1);		// 弾数５０％アップ
		break;
	}

	TamaCmd.ns++;		// 連射数_増加
}

static void luna_cmd(TAMA_CMD& TamaCmd)
{
	switch(TamaCmd.cmd&0x03){
		case(TC_WAY):
			TamaCmd.n+=4;					// 奇数・偶数は変化させない
			TamaCmd.dw -= (TamaCmd.dw/3);	// 幅を狭める
		break;

		case(TC_ALL):
			TamaCmd.n += (( (TamaCmd.n/3)>12 ) ? 12 : (TamaCmd.n/3) );
		break;

		case(TC_RND):
			TamaCmd.n<<=1;					// 弾数２倍
		break;
	}

	TamaCmd.ns+=2;		// 連射数_増加
}

uint8_t tama_dir(uint16_t i)
{
	uint8_t deg = ((TamaCmd.cmd & TAMA_ZSET)
		? atan8((Viv.x - TamaCmd.x), (Viv.y - TamaCmd.y))
		: 0
	);

	deg += TamaCmd.d;	// 基本角のセット完了
	i = i%TamaCmd.n;	// 連射弾対策

	switch(TamaCmd.cmd & 0x03){
		case(TC_WAY):
			i++;
			if(TamaCmd.n&1)
				return deg+(i>>1)*TamaCmd.dw*(1-((i&1)<<1));
			else
				return deg-(TamaCmd.dw>>1)+(i>>1)*TamaCmd.dw*(1-((i&1)<<1));

		case(TC_ALL):
			return deg + (i<<8)/TamaCmd.n;

		case(TC_RND):
			// DebugOut(u8"1");
			return (deg + (RNG.next() % TamaCmd.dw) - (TamaCmd.dw >> 1));

		default:
			return 0;	// 絶対無いけれど、warning がうるさいので...
	}
}

int LineCmdNewTamaSpeed(uint16_t i)
{
	int		vret = TamaSpeed;		// 速度の基本値をセットする(GIAN.H)

	i = (i%TamaCmd.n)+1;	// 連射弾対策

	// 中心からの角度
	const auto deg_factor = ((i >> 1) * TamaCmd.dw * (1 - ((i & 1) << 1)));
	const uint8_t deg = ((TamaCmd.n & 1)
		? deg_factor
		: -(TamaCmd.dw >> 1) + deg_factor
	);

	vret = cosDiv(deg, vret);

	if(TamaCmd.cmd & TAMA_REN)
		return vret + (vret>>3)*(i-1);
	else
		return vret;
}

int tama_speed(uint16_t i, int vret, const TAMA_CMD& TamaCmd)
{
	int temp = 0;						// ランダム要素の設定用

	// 速度ランダムは基本値のｎ％変化とするべきかもしれないが... //
	switch(TamaCmd.v & 0xc0){
	case(TAMASP_RND1): temp = ((RNG.next() % 16) -  8); break;
	case(TAMASP_RND2): temp = ((RNG.next() % 32) - 16); break;
	case(TAMASP_RND3): temp = ((RNG.next() % 64) - 32); break;
	}

	if(TamaCmd.cmd & TAMA_REN)
		return vret + (vret>>3)*(i/TamaCmd.n) + temp;
	else
		return vret + temp;
}

uint8_t tama_flag(void)
{
	switch(TamaCmd.type){
		case(T_HOMING):case(T_HOMING_M):
		case(T_ROLL):case(T_ROLL_A):case(T_ROLL_R):case(T_SBHOMING):
		return TF_CLIP;

		default:
		return TF_NONE;
	}
}

void TamaSetSingle(
	TAMA_DATA& t, unsigned int i, int speed, decltype(TAMA_DATA::effect) effect
)
{
	t.x = t.tx = TamaCmd.x;	// X座標のセット
	t.y = t.ty = TamaCmd.y;	// Y座標のセット

	//temp = random_ref;
	t.v = t.v0 = tama_speed(i, speed, TamaCmd); // 初速度のセット
	//Debug(temp,30);

	t.a = TamaCmd.a;	// 注意：サイズは char

	//temp = random_ref;
	t.d   = tama_dir(i);	// 弾の発射角度
	//Debug(temp,31);

	t.d16 = (t.d << 8);	// 角速度のある運動で使用

	t.vx = cosl(t.d, t.v);	// 速度のＸ成分セット
	t.vy = sinl(t.d, t.v);	// 速度のＹ成分セット

	t.vd     = TamaCmd.vd;	// 角速度もしくはホーミング率
	t.c      = TamaCmd.c;	// 弾の色＆形状
	t.rep    = TamaCmd.rep;	// 繰り返し回数
	t.type   = TamaCmd.type;	// 弾の種類
	t.option = TamaCmd.option;	// 弾の属性(バイブ、反射等)
	t.effect = effect;	// 弾のエフェクト
	t.count  = 0;	// カウンタの初期化
	t.flag   = tama_flag();	// フラグの初期化
}

void tamaTmove(TAMA_DATA *t)
{
	short		deg_t;
	//ENEMY_DATA	*e;

	// (x,y)に直接アクセスするのではなく、(tx,ty)にアクセスする事！ //
	switch(t->type & 0x0f){
		case(T_NORM):		// 通常弾
			//MMX_ADD32(&t->tx,&t->vx);
			t->tx += t->vx;
			t->ty += t->vy;
		return;

		case(T_NORM_A):		// 加速弾
			t->v  += t->a;
			t->tx += cosl(t->d,t->v);
			t->ty += sinl(t->d,t->v);
			if(t->rep == t->count){
				t->type = (t->type&0xf0) | T_NORM;	// 上位ビットは一応保存する
				t->vx   = cosl(t->d,t->v);
				t->vy   = sinl(t->d,t->v);
			}
		return;

		case(T_HOMING):		// ｎ回ホーミング
			t->v  += t->a;
			t->tx += cosl(t->d,t->v);
			t->ty += sinl(t->d,t->v);
			if( (t->a>0) && (t->v>=t->v0) ){
				t->a = -(t->a);
				if(--(t->rep)==0){
					t->type = (t->type&0xf0) | T_NORM;
					t->flag &= (~TF_CLIP);
					t->vx   = cosl(t->d,t->v);
					t->vy   = sinl(t->d,t->v);
				}
			}
			if( (t->a<0) && (t->v<=0 ) ){
				t->a  = -(t->a);
				t->d  = atan8((Viv.x)-(t->x),(Viv.y)-(t->y));
			}
		return;

		case(T_HOMING_M):	// ｎ％ホーミング(ミサイル系？)
			// 最適化はしておりませんな... //
			if( (t->count>19) && (t->count%2==0) ){
				deg_t=atan8((Viv.x)-(t->x),(Viv.y)-(t->y))-(t->d);
				if(deg_t<-128) deg_t += 256;
				if(deg_t> 128) deg_t -= 256;
				t->d = t->d + deg_t*(t->vd)/255;
			}
			t->v  += t->a;
			t->tx += cosl(t->d,t->v);
			t->ty += sinl(t->d,t->v);
			if(t->rep == t->count){
				t->type = (t->type&0xf0) | T_NORM;	// 上位ビットは一応保存する
				t->flag &= (~TF_CLIP);
				t->vx   = cosl(t->d,t->v);
				t->vy   = sinl(t->d,t->v);
			}
		return;

		case(T_ROLL):		// 回転弾
			t->d += Cast::sign<uint8_t>(t->vd);
			t->tx += cosl(t->d,t->v);
			t->ty += sinl(t->d,t->v);
			if(t->rep == t->count){
				t->type = (t->type&0xf0) | T_NORM;	// 上位ビットは一応保存する
				t->flag &= (~TF_CLIP);
				t->vx   = cosl(t->d,t->v);
				t->vy   = sinl(t->d,t->v);
			}
		return;

		case(T_ROLL_A):		// 回転弾(加速) 最初の加速度は"負"にして下さい！
			t->v  += t->a;
			if(t->a > 0) {
				t->d += Cast::sign<uint8_t>(t->vd);
			}
			t->tx += cosl(t->d,t->v);
			t->ty += sinl(t->d,t->v);
			if( (t->a<0) && (t->v<=0 ) ) t->a = -(t->a);
			if( (t->a>0) && (t->v>=t->v0) ){
				t->a = -(t->a);
				if(--(t->rep)==0){
					t->type = (t->type&0xf0) | T_NORM;
					t->flag &= (~TF_CLIP);
					t->vx   = cosl(t->d,t->v);
					t->vy   = sinl(t->d,t->v);
				}
			}
		return;

		case(T_ROLL_R):		// 回転弾(反転) 上と同じで加速度に注意！
			t->v += t->a;
			t->d += Cast::sign<uint8_t>(t->vd);
			t->tx += cosl(t->d,t->v);
			t->ty += sinl(t->d,t->v);
			if( (t->a<0) && (t->v<=0 ) ){
				t->d   += 128;
				t->a    = -(t->a);
			}
			if( (t->a>0) && (t->v>=t->v0) ){
				t->a = -(t->a);
				if(--(t->rep)==0){
					t->type = (t->type&0xf0) | T_NORM;
					t->flag &= (~TF_CLIP);
					t->vx   = cosl(t->d,t->v);
					t->vy   = sinl(t->d,t->v);
				}
			}
		return;

		case(T_GRAVITY):	// 落下弾(上昇弾にもできるが...)
			t->vy += t->a;
			//MMX_ADD32(&t->tx,&t->vx);
			t->tx += t->vx;
			t->ty += t->vy;
		return;

		case(T_CHANGE):		// 角度強制変更弾
			//MMX_ADD32(&t->tx,&t->vx);
			t->tx += t->vx;
			t->ty += t->vy;
			if(t->rep == t->count){
				t->type = (t->type&0xf0) | T_NORM;	// 上位ビットは一応保存する
				t->d    = Cast::sign<uint8_t>(t->vd);
				t->vx   = cosl(t->d,t->v);
				t->vy   = sinl(t->d,t->v);
			}
		return;

		case(T_SBHOMING):	// サボテン用ホーミング(煙を吐き出すぞ！)
			if(t->count & 1) {
				Fragment.Set(t->x, t->y, FRG_SMOKE);
			}
			t->tx += t->vx;
			t->ty += t->vy;
			if((t->count<130-60) && HomingFlag!=HOMING_DUMMY){
				deg_t=atan8(HomingX-(t->x),HomingY-(t->y))-(t->d);
			}
			else if(t->count<130-60){
				deg_t=atan8(0,(-20*64)-(t->y))-(t->d);
			}
			else{
				t->flag = TF_NONE;
				deg_t=0;
			}

			if(deg_t<-128) deg_t += 256;
			if(deg_t> 128) deg_t -= 256;
			//if(deg_t>-2 && deg_t<2){
			if(deg_t==0){
				if(t->vd) t->vd--;
				t->v += t->a;
			}
			else{
				// pbg quirk: Was probably intended to compare the unsigned
				// representation against 0xFA? Since `t->vd` is a `int8_t`,
				// this is always true. Visual Studio 2022 optimizes this
				// comparison away in Release mode, Clang throws a warning.
				//
				// 	if(t->vd<250)
				t->vd++;
				t->v -= t->a;
			}
			t->d += (deg_t * (Cast::sign<uint8_t>(t->vd)) / 255);
			t->vx = cosl(t->d,t->v);
			t->vy = sinl(t->d,t->v);
		return;

		case(T_SBHBOMB):	// サボテン用ホーミングボム
			// ちゅうい : この case はダミーです決して実行されてはいけません //
			if(t->count>=49) t->flag = TF_DELETE;
		return;
	}
}

void tamaOmove(TAMA_DATA *t)
{
	int op_temp = 0;

	// 分裂はとボムは消去要請フラグを立てる必要がある //
	// (x,y)に(tx,ty)の演算結果を利用して、値を代入する //
	switch(t->option & 0xf0){
		case(TOP_NONE):		// オプション無し
			t->x = t->tx;
			t->y = t->ty;
		return;

		case(TOP_WAVE):		// 波
			op_temp = sinl(
				Cast::down_sign<uint8_t>(t->count << 2),
				((t->option & 0x0f) << 7)
			);
			t->x    = t->tx - sinl(t->d,op_temp);
			t->y    = t->ty + cosl(t->d,op_temp);
		return;

		case(TOP_ROLL): { // 回転
			const auto angle = Cast::down_sign<uint8_t>(t->d + (t->count << 1));
			op_temp = (t->option&0x0f)<<8;
			t->x    = (t->tx + cosl(angle, op_temp));
			t->y    = (t->ty + sinl(angle, op_temp));
		}
		return;

		case(TOP_PURU):		// ぷるぷる
		return;

		case(TOP_REFX):		// 反射Ｘ
			if((t->tx)<GX_MIN||(t->tx)>GX_MAX){
				t->d  = 128 - t->d;
				t->vx = -(t->vx);
				t->x  = t->tx + cosl(t->d,t->v);
				t->y  = t->ty + sinl(t->d,t->v);
				op_temp = (t->option&0x0f);
				if(op_temp == 0)
					t->option = TOP_NONE;
				else
					t->option = TOP_REFX | (op_temp-1);
			}
			else{
				t->x = t->tx;
				t->y = t->ty;
			}
		return;

		case(TOP_REFY):		// 反射Ｙ
			if((t->ty)<GY_MIN){
				t->d  = - t->d;
				t->vy = -(t->vy);
				t->x  = t->tx + cosl(t->d,t->v);
				t->y  = t->ty + sinl(t->d,t->v);
				op_temp = (t->option&0x0f);
				if(op_temp == 0)
					t->option = TOP_NONE;
				else
					t->option = TOP_REFY | (op_temp-1);
			}
			else{
				t->x = t->tx;
				t->y = t->ty;
			}
		return;

		case(TOP_REFXY):	// 反射ＸＹ
			if((t->tx)<GX_MIN||(t->tx)>GX_MAX){
				t->d  = 128 - t->d;
				t->vx = -(t->vx);
				t->x  = t->tx + cosl(t->d,t->v);
				t->y  = t->ty + sinl(t->d,t->v);
				op_temp = (t->option&0x0f);
				if(op_temp == 0)
					t->option = TOP_NONE;
				else
					t->option = TOP_REFXY | (op_temp-1);
			}
			else if((t->ty)<GY_MIN){
				t->d  = - t->d;
				t->vy = -(t->vy);
				t->x  = t->tx + cosl(t->d,t->v);
				t->y  = t->ty + sinl(t->d,t->v);
				op_temp = (t->option&0x0f);
				if(op_temp == 0)
					t->option = TOP_NONE;
				else
					t->option = TOP_REFXY | (op_temp-1);
			}
			else{
				t->x = t->tx;
				t->y = t->ty;
			}
		return;

		case(TOP_DIV):		// 分裂
			t->x = t->tx;
			t->y = t->ty;
			if((t->tx)<GX_MIN||(t->tx)>GX_MAX){
				op_temp   = 1;
				TamaCmd.d = 128 - (t->d);
			}
			else if((t->ty)<GY_MIN){
				op_temp   = 1;
				TamaCmd.d = - (t->d);
			}

			if(op_temp == 1){
				TamaCmd.x = t->tx + cosl(TamaCmd.d,t->v);
				TamaCmd.y = t->ty + sinl(TamaCmd.d,t->v);
				t->flag   = TF_DELETE;			// 消滅エフェクトに変更すべきか？
				TamaCmd.ns = 2;
				TamaCmd.c = (t->c)&0x0f;
				TamaCmd.cmd = (t->option & 0x0f) | TE_CIRCLE1;
				switch(TamaCmd.cmd&0x03){
					case(TC_WAY):
						TamaCmd.n = 3;
						TamaCmd.dw = 16;
						TamaCmd.v  = 13-2;
					break;
					case(TC_ALL):
						TamaCmd.n  = 10;
						TamaCmd.v  = 13;
						TamaCmd.d  = Cast::down<uint8_t>(RNG.next());
						if(TamaCmd.cmd&TAMA_REN) TamaCmd.v -= 2;
					break;
					case(TC_RND):
						TamaCmd.n = 4;
						TamaCmd.dw = 128-32;		// 128以上だと画面外に...
						TamaCmd.v  = 13|TAMASP_RND2;	// 速度ランダムあり
					break;
				}
				if(TamaCmd.cmd&TAMA_ZSET) TamaCmd.d=0,TamaCmd.dw-=6;
				TamaCmd.type = T_NORM;
				TamaCmd.option = TOP_NONE;
				Snd_SEPlay(12, TamaCmd.x);
				tama_set();		// 難易度で変化させるところがポイント
			}
		return;

		case(TOP_BOMB):		// ボム
		return;
	}
}

void tamaEmove(TAMA_DATA *t)
{
	// TE_NONE:エフェクト無しはこの関数にこないので記述しても意味無し //
	// TE_DELETE:消去要請フラグを立てる事を忘れないように！ //
	switch(t->effect & 0xf0){
		case(TE_ROLL1):
		return;

		case(TE_ROLL2):
		return;

		case(TE_WARN):
		return;

		case(TE_ROCK):
		return;

		case(TE_CIRCLE1):
			t->x = (t->tx += (t->vx>>1));
			t->y = (t->ty += (t->vy>>1));

			if(t->count>=5*4-1){
				t->effect = 0;
			}
		return;

		case(TE_CIRCLE2):
		return;

		case(TE_DELETE):
			t->x += (t->vx>>1);
			t->y += (t->vy>>1);
			//t->d+=4;
			if(t->count>=47){
				t->flag = TF_DELETE;
			}
		return;
	}
}

void ResetHoming(void)
{
	HomingFlag = HOMING_DUMMY;
}

void UpdateHoming(const WORLD_POINT& p)
{
	const int temp = (Viv.y - p.y);
	if(temp < 0) {
		return;
	}
	if(temp < HomingFlag) {
		HomingFlag = temp;
		HomingX = p.x;
		HomingY = p.y;
	}
}
