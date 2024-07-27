/*                                                                           */
/*   Boss.cpp   ボスの処理(中ボス含む)                                       */
/*                                                                           */
/*                                                                           */

#include "BOSS.H"
#include "BOMBEFC.H" // 爆発エフェクト処理
#include "GEOMETRY.H"
#include "GIAN.H"
#include "EnemyExCtrl.h"
#include "platform/graphics_backend.h"
#include "game/cast.h"
#include "game/snd.h"
#include "game/ut_math.h"


///// [ 定数 ] /////

// 最大数 //
#define BOSS_MAX	4	// ボスの最大出現数(二匹以上出るのか？？)

// ボスの状態 //
#define BEXST_NORM	0x00	// 通常のＥＣＬで動作中
#define BEXST_DEAD	0x01	// 死亡中<-こいつは多分使っていないぞ(2000/10/31)
#define BEXST_WING01	0x02	// 蝶の羽
#define BEXST_WING02	0x03	// 天使の羽
#define BEXST_SHILD1	0x04	// シールド１
#define BEXST_SHILD2	0x05	// シールド２

// 体力ゲージ編 //
#define BOSSHPG_WIDTH	256	// 体力ゲージの幅
#define BOSSHPG_HEIGHT	24	// 体力ゲージの高さ
#define BOSSHPG_START_X	X_MAX	// 体力ゲージの初期Ｘ
#define BOSSHPG_END_X	260	// 体力ゲージの最終Ｘ

#define BHPG_DEAD	0x00	// 体力ゲージは使用されていない
#define BHPG_OPEN1	0x01	// 体力ゲージを開く(第一エフェクト中)
#define BHPG_OPEN2	0x02	// 体力ゲージを開く(体力上昇中)
#define BHPG_NORM	0x03	// 体力ゲージの準備ができている
#define BHPG_CLOSE	0x04	// 体力ゲージを閉じる
#define BHPG_OPEN3	0x05	// 体力ゲージを更新する



///// [構造体] /////

// ボスの体力ゲージ //
typedef struct tagBOSSHPG_INFO{
	uint32_t	Now, Max;	// 体力の現在値＆最大値
	uint32_t	Next;	// 次の体力の値
	uint32_t	Update;	// 更新用の値
	uint32_t	Count;	// フレーム数保持

	uint16_t	XTemp[BOSSHPG_HEIGHT];	// ＨＰゲージの演出用
	uint8_t	State;	// 状態
} BOSSHPG_INFO;



///// [ 変数 ] /////

// 公開グローバル //
BOSS_DATA		Boss[BOSS_MAX];		// ボスデータ格納用構造体
uint16_t	BossNow;	// 現在のボスの数


// 秘密のグローバル //
BOSSHPG_INFO	BossHPG;			// 体力ゲージ保持用



// 秘密の関数 //
static void BossHPG_Open(uint32_t max);	// ボスの体力ゲージをオープンする
static void BossHPG_Move(uint32_t now);	// ボスの体力ゲージを増減する
static void BossHPG_Close(void);	// ボスの体力ゲージをクローズする
static void BossHPG_Update(uint32_t next);	// ボスの体力ゲージを上昇させる

static int PutBoss(int x, int y, uint32_t id);	// ボスをセットする
static void BossSTDMove(BOSS_DATA *b);	// ノーマルECL互換の移動



// ボスデータ配列を初期化する(中断、ステージクリア時に使用) //
void BossDataInit(void)
{
	for(auto& it : Boss) {
		it.IsUsed   = 0;	// 要はこれをゼロにすれば良い

		// 特殊変数の初期化.... //
		it.ExMove   = BossSTDMove;	// 特殊移動関数
		it.ExCount  = 0;
		it.ExState  = BEXST_NORM;	// 特殊状態変数
		it.Hit      = nullptr;	// 特殊当たり判定
	}

	// ボスが死んでいるのだから、体力ゲージはもちろん表示しない //
	BossHPG.State = BHPG_DEAD;

	// ボスの数を０にする //
	BossNow = 0;

	// 蛇管理を初期化(やや謎) //
	SnakyInit();

	// ビット管理も初期化 //
	BitInit();
}

// ボスをセットする //
void BossSet(int x, int y, uint32_t BossID)
{
	int n;
	uint32_t HP_Sum = 0;

	// ｘ６４座標に変換してボスをセットする //
	n = PutBoss(x<<6,y<<6,BossID);

	if(n==BOSS_MAX) return;		// ここに来たらバグ

	Boss[n].ExCount = 0;
	Boss[n].ExMove  = BossSTDMove;
	Boss[n].ExState = BEXST_NORM;
	Boss[n].IsUsed  = 0xff;

	parse_ECL(&(Boss[n].Edat));
	//ObjectLockOn(&(Boss[n].Edat.x),&(Boss[n].Edat.y),Boss[n].Edat.g_width,Boss[n].Edat.g_height);

	for(const auto& it : Boss) {
		if(it.IsUsed) {
			HP_Sum += it.Edat.hp;
		}
	}

	BossHPG_Open(HP_Sum);
	BossNow++;
}


// ボスをセットする(ＥＣＬ用) //
void BossSetEx(int x, int y, uint32_t BossID)
{
	int n;
	uint32_t HP_Sum = 0;

	// ｘ６４座標に変換してボスをセットする //
	n = PutBoss(x<<6,y<<6,BossID);

	if(n==BOSS_MAX) return;		// ここに来たらバグ

	Boss[n].ExCount = 0;
	Boss[n].ExMove  = BossSTDMove;
	Boss[n].ExState = BEXST_NORM;
	Boss[n].IsUsed  = 0xff;

	parse_ECL(&(Boss[n].Edat));
	//ObjectLockOn(&(Boss[n].Edat.x),&(Boss[n].Edat.y),Boss[n].Edat.g_width,Boss[n].Edat.g_height);

	for(const auto& it : Boss) {
		if(it.IsUsed) {
			HP_Sum += it.Edat.hp;
		}
	}

	BossHPG_Update(HP_Sum);
	BossNow++;
}


// ボスを動かす //
void BossMove(void)
{
	uint32_t HP_Sum = 0;
	ENEMY_DATA		*e;

	HomingFlag = HOMING_DUMMY;

	for(auto& it : Boss) {
		auto *b = &it;
		if(b->IsUsed){
			e = &(b->Edat);
			e->IsDamaged = 0;
			b->ExMove(b);

			// サボテンヒットチェック //
			if(HITCHK(e->x,Viv.x,e->g_width) && HITCHK(e->y,Viv.y,e->g_height) && Viv.muteki==0){
				// ここら辺で敵にダメージを与えるとおもしろいかも？ //
				if(e->flag&EF_HITSB) MaidDead();
			}

			// ホーミングの準備 //
			if(e->flag&EF_DAMAGE) UpdateHoming(e);

			// 体力の総和を表示する //
			HP_Sum += b->Edat.hp;

			// アニメーションの動作 //
			EnemyAnimeMove(e);

			e->count++;
		}
	}

	SnakyMove();
	BitMove();
	BossHPG_Move(HP_Sum);
}

// ボスを描画する
void BossDraw(void)
{
	int x, y;
	int			w,h,t;
	ENEMY_DATA	*e;
	PIXEL_LTRB	wing;
/*
	PIXEL_LTRB	Right[6] = {
		{240+40, 400, 240+80, 480}, {320, 400, 320+32, 480},
		{384+24, 400, 384+48, 480}, {432+8, 400, 432+16, 480},
		{464+8,  400, 464+16, 480}, {480+8, 400, 480+16,  480},
	};

	PIXEL_LTRB	Left[6] = {
		{240, 400, 240+40, 480}, {320, 400, 320+32, 480},
		{384, 400, 384+48, 480}, {432, 400, 432+16, 480},
		{464, 400, 464+16,  480}, {480, 400, 480+16,  480},
	};

	WORD	size[6] = {
		40, 32, 24, 16, 8, 8
	};
*/
	BitLineDraw();

	for(auto& it : Boss) {
		auto *b = &it;
		if(b->IsUsed){
			e = &(b->Edat);

			x = (e->x>>6);
			y = (e->y>>6);

			// 霊魂状態 //
			if(b->ExState == BEXST_SHILD2 && Viv.bomb_time && (e->flag&EF_DRAW)){
				wing = PIXEL_LTWH{
					(160 + ((e->count / 2) % 4) * 40), 80, 40, 40
				};
				GrpBltX(wing, (x - 20), (y - 20), GrEnemy); // クリッピングなし
				continue;
			}

			// バリア状態 //
			if(b->ExState == BEXST_SHILD1 && Viv.bomb_time && (e->flag&EF_DRAW)){
				GrpGeom->Lock();
				for(uint8_t j = 0; j <= 5; j++) {
					GrpGeom->SetColor({ (5u - j), (5u - j), 5u });
					GeomCircle(
						{ x, y }, (sinl((e->count * 4), (30 + (j * 4))) + 80)
					);
				}
				GrpGeom->Unlock();
			}

			switch(b->ExState){
				case(BEXST_WING01):
					t = (b->ExCount-64-8)<<2;
					if(t<0) t=0;
					w = 64;
					h = 92;
					wing = {   0, 176, 128, 360 };
					GrpBlt(&wing, x-w-t, y-h, GrEnemy);
					wing = { 128, 176, 256, 360 };
					GrpBlt(&wing, x-w+t, y-h, GrEnemy);
				break;

				case(BEXST_WING02):
					w = 44;
					h = 52;
					wing = { 552,   0, 640, 104 };
					GrpBlt(&wing, x-w-50, y-h, GrEnemy);
					wing = { 552, 104, 640, 208 };
					GrpBlt(&wing, x-w+50, y-h, GrEnemy);
				break;
			}

			if(e->flag&EF_DRAW){
				e->Draw();
			}
		}
	}
}


// ボス用・敵弾クリアの前処理関数 //
void BossClearCmd(void)
{
	BitDelete();
}


// ボスの体力ゲージをオープンする //
static void BossHPG_Open(uint32_t max)
{
	int		i;

	BossHPG.Max    = max;		// 最大値
	BossHPG.Now    = 0;			// 最初のエフェクトで上昇して行くので
	BossHPG.Next   = max;		// 次の体力値
	BossHPG.Update = max;		// 更新用の値

	BossHPG.State = BHPG_OPEN1;
	BossHPG.Count = 0;

	// 表示用初期Ｘを指定する(乱数を使用するが...) //
	for(i=0;i<BOSSHPG_HEIGHT;i++){
		BossHPG.XTemp[i] = BOSSHPG_START_X+i*20;
	}
}


// ボスの体力ゲージを上昇させる //
static void BossHPG_Update(uint32_t next)
{
//	BossHPG.Max  = max;		// 最大値
//	BossHPG.Now  = 0;		// 最初のエフェクトで上昇して行くので
	BossHPG.Update = next;	// 次の体力値

	BossHPG.State = BHPG_OPEN3;
//	BossHPG.Count = 0;
}


// ボスの体力ゲージを増減する //
static void BossHPG_Move(uint32_t now)
{
	int		i;
	int		ChkCount = 0;

	BossHPG.Next = now;

	switch(BossHPG.State){
		case(BHPG_OPEN1): {
			for(auto& it : BossHPG.XTemp) {
				it -= 6;
				if(it <= BOSSHPG_END_X){
					it = BOSSHPG_END_X;
					ChkCount++;
				}
			}

			if(ChkCount==BOSSHPG_HEIGHT) BossHPG.State = BHPG_OPEN2;
		}
		break;

		case(BHPG_OPEN2):
			BossHPG.Now += ((BossHPG.Max>>7)+1);
			if(BossHPG.Now>=BossHPG.Max){
				BossHPG.Now   = BossHPG.Max;
				BossHPG.State = BHPG_NORM;
			}
		break;

		case(BHPG_OPEN3):
			BossHPG.Now += ((BossHPG.Max>>7)+1);
			if(BossHPG.Now >= BossHPG.Update){
				BossHPG.Now   = BossHPG.Update;
				BossHPG.State = BHPG_NORM;
			}
		break;

		case(BHPG_NORM):
			if(BossHPG.Now > BossHPG.Next){
				//temp = max(BossHPG.Max>>10,1);
				//temp = max((30*8*3)/max(BossHPG.Max,1),3);
				const auto temp = (std::max)(
					((std::max)(BossHPG.Max, 1u) / (30 * 8 * 4)), 3u
				);
				if(BossHPG.Now-BossHPG.Next>temp) BossHPG.Now -= temp;
				else                              BossHPG.Now = BossHPG.Next;
			}
			if(BossHPG.Now==0) BossHPG_Close();
		break;

		case(BHPG_CLOSE):
			BossHPG.XTemp[BOSSHPG_HEIGHT-1] += 6;
			for(i=BOSSHPG_HEIGHT-2;i>=0;i--){
				BossHPG.XTemp[i] = max(BossHPG.XTemp[i],BossHPG.XTemp[i+1]-20);
			}
			if(BossHPG.XTemp[0]>=BOSSHPG_START_X)
				BossHPG_Close();
		break;

		case(BHPG_DEAD):
			// もちろん何もしない //
		return;
	}

	BossHPG.Count++;
}

// ボスの体力ゲージをクローズする //
static void BossHPG_Close(void)
{
	// 後で変更のこと //
	BossHPG.State = BHPG_CLOSE;
}

// ボスの体力ゲージを描画する //
void BossHPG_Draw(void)
{
	PIXEL_LTRB	src;
	int i;

	switch(BossHPG.State){
		case(BHPG_OPEN1):case(BHPG_CLOSE):
			// エフェクト付き枠の描画 //
			for(i=0;i<BOSSHPG_HEIGHT;i++){
				src = { 0, (104 + i), BOSSHPG_WIDTH, (104 + i + 1) };
				GrpBlt(&src,BossHPG.XTemp[i],16+i,GrTama);
			}
		break;

		case(BHPG_OPEN2): case(BHPG_NORM): case(BHPG_OPEN3): {
			// 体力ゲージの描画 //
			constexpr WINDOW_COORD left   = (BOSSHPG_END_X + 3);
			constexpr WINDOW_COORD top    = (16 + 3);
			constexpr WINDOW_COORD bottom = (top + 11);
			const auto x1 = (left + ((BossHPG.Next * 30 * 8) / BossHPG.Max));
			const auto x2 = (left + ((BossHPG.Now  * 30 * 8) / BossHPG.Max));
			constexpr uint8_t alpha = (128 + 64);
			constexpr RGB216 col    = { 0, 1, 5 };

			GrpGeom->Lock();
			GrpGeom->SetAlphaNorm(alpha);
			if(auto *gp = GrpGeom_Poly()) {
				VERTEX_XY Src[4] = {
					{ 0, top },
					{ left, top },
					{ left, bottom },
					{ 0, bottom },
				};
				if(x1<x2){
					Src[0].x = Src[3].x = x1;
					GeomGrdRectA(*gp, Src, col.ToRGB().WithAlpha(alpha));
					// gp->DrawBoxA(left, top, x1, bottom);
					gp->SetColor({ 5, 0, 0 });
					gp->DrawBoxA(x1, top, x2, bottom);
				}
				else{
					Src[0].x = Src[3].x = x2;
					GeomGrdRectA(*gp, Src, col.ToRGB().WithAlpha(alpha));
					// gp->DrawBoxA(left, top, x2, bottom);
				}
			}
			else{
				constexpr auto line_top = (top + 5);
				constexpr auto line_bottom = (bottom - 4);
				GrpGeom->SetColor(col);
				if(x1<x2){
					GrpGeom->DrawBoxA(left, top, x2, line_top);
					GrpGeom->DrawBoxA(left, line_bottom, x2, bottom);
					GrpGeom->SetColor({ 5, 5, 5 });
					GrpGeom->DrawBoxA(left, line_top, x1, line_bottom);
					GrpGeom->SetColor({ 5, 0, 0 });
					GrpGeom->DrawBoxA(x1, top, x2, bottom);
				}
				else{
					GrpGeom->DrawBoxA(left, top, x2, line_top);
					GrpGeom->DrawBoxA(left, line_bottom, x2, bottom);
					GrpGeom->SetColor({ 5, 5, 5 });
					GrpGeom->DrawBoxA(left, line_top, x2, line_bottom);
				}
			}

			GrpGeom->Unlock();

			// 枠の描画 //
			src = { 0, 104, BOSSHPG_WIDTH, 128 };
			GrpBlt(&src,BOSSHPG_END_X,16,GrTama);
		}
		break;

		case(BHPG_DEAD):
			// もちろん何もしない //
		break;
	}
}

// 現在出現しているボス全てのＨＰを０にする //
void BossKillAll(void)
{
	// 破壊後の破片放出などの処理は、ダメージを与えて破壊するのと同等の関数を //
	// 使用するが、当然のごとく、得点＆経験値？は入手できない                 //
	// レーザークローズも忘れずに！！                                         //

	ENEMY_DATA		*e;

	for(auto& it : Boss) {
		auto *b = &it;
		if(b->IsUsed){
			e = &(b->Edat);
			SnakyDelete(b);
			fragment_set(e->x,e->y,FRG_FATCIRCLE);
			ExBombEfcSet(e->x, e->y, EXBOMB_STD);
			Snd_SEPlay(SOUND_ID_BOSSBOMB, e->x);
			if(e->LLaserRef) LLaserForceClose(e);	// レーザーの強制クローズ
			e->hp = 0;
			e->count = 0;
			e->flag = EF_BOMB;
			b->IsUsed = 0;
		}
	}

	BossNow = 0;
}

bool BossDamageApply(BOSS_DATA& b, ENEMY_DATA& e, int damage)
{
	e.IsDamaged = ((e.count) & 1);
	if(e.hp <= damage) { // ボスの死亡処理(後で変更すること!!)
		SnakyDelete(&b);
		BitDelete();
		enemy_clear();
		fragment_set(e.x, e.y, FRG_FATCIRCLE);
		ExBombEfcSet(e.x, e.y, EXBOMB_STD);
		ScrollCommand(SCMD_QUAKE);
		Snd_SEPlay(SOUND_ID_BOSSBOMB, e.x);
		if(e.LLaserRef) {
			LLaserForceClose(&e); // レーザーの強制クローズ
		}
		PowerUp(Cast::down<uint8_t>(e.hp));
		e.hp    = 0;
		e.count = 0;
		e.flag  = EF_BOMB;

		// 最後の一匹だった場合 //
		if(BossNow == 1) {
			char buf[100];
			const auto temp = tama2score(); // 弾→スコアエフェクト
			// sprintf(buf, "%3d Evade  %5dPts", Viv.evade, Viv.evadesc);
			sprintf(buf, "  Bonus    %7uPts", temp);
			StringEffect(180, 60, buf);
			score_add(temp);
		}

		if(e.item) {
			ItemSet(e.x, e.y, e.item);
		}
		score_add(e.score);
		laser_clear();
		b.IsUsed = 0;
		BossNow--; // ボスの参照カウント？を使用する
	} else {
		Snd_SEPlay(SOUND_ID_HIT, e.x);
		PowerUp(damage);
		e.hp -= damage;
	}
	return true;
}

// ボスにダメージを与える //
bool BossDamage(int x, int y, int damage)
{
	int				i;
	ENEMY_DATA		*e;

	i = (BitGetNum()>>1);
	damage -= i;
	if(damage <= 0) {
		return false;
	}

	for(auto& it : Boss) {
		auto *b = &it;
		if(b->ExState == BEXST_SHILD1 || b->ExState == BEXST_SHILD2){
			if(Viv.bomb_time) continue;
		}

		if(b->IsUsed){
			e = &(b->Edat);
			if(HITCHK(x,e->x,e->g_width) && HITCHK(y,e->y,e->g_height) && (e->flag&EF_DAMAGE)){
				if(e->flag==EF_BOMB || !(e->flag&EF_DAMAGE)) continue;
				else{
					return BossDamageApply(*b, *e, damage);
				}
			}
		}
	}
	return false;
}

// ボスにダメージを与える(ｙ上方向無限Ver) //
bool BossDamage2(int x, int y, int damage)
{
	int				i;
	ENEMY_DATA		*e;
	bool	ret_val = false;

	i = (BitGetNum()>>1);
	damage -= i;
	if(damage <= 0) {
		return false;
	}

	for(auto& it : Boss) {
		auto *b = &it;
		if(b->ExState == BEXST_SHILD1 || b->ExState == BEXST_SHILD2){
			if(Viv.bomb_time) continue;
		}

		if(b->IsUsed){
			e = &(b->Edat);
			if(HITCHK(x,e->x,e->g_width) && (y > e->y) && (e->flag&EF_DAMAGE)){
				if(e->flag==EF_BOMB || !(e->flag&EF_DAMAGE)) continue;
				else{
					ret_val = BossDamageApply(*b, *e, damage);
				}
			}
		}
	}
	return ret_val;
}

// ボスにダメージを与える(ナナメレーザー) //
void BossDamage3(int x, int y, uint8_t d)
{
	int				i;
	ENEMY_DATA		*e;
	//BOOL			ret_val = FALSE;
	int				damage  = 2;

	i = (BitGetNum()>>1);
	damage -= i;
	if(damage <= 0) return;

	for(auto& it : Boss) {
		auto *b = &it;
		if(b->ExState == BEXST_SHILD1 || b->ExState == BEXST_SHILD2){
			if(Viv.bomb_time) continue;
		}

		if(b->IsUsed){
			e = &(b->Edat);
			if(LaserHITCHK(e, x, y, d) && (e->flag&EF_DAMAGE)){
				if(e->flag==EF_BOMB || !(e->flag&EF_DAMAGE)) continue;
				else{
					BossDamageApply(*b, *e, damage);
				}
			}
		}
	}
}


// ボスにダメージを与える(すべての敵)
void BossDamage4(int damage)
{
	int				i;
	ENEMY_DATA		*e;

	i = (BitGetNum()>>1);
	damage -= i;
	if(damage <= 0) return;

	for(auto& it : Boss) {
		auto *b = &it;
		if(b->ExState == BEXST_SHILD1 || b->ExState == BEXST_SHILD2){
			if(Viv.bomb_time) continue;
		}

		if(b->IsUsed){
			e = &(b->Edat);
			if(e->flag&EF_DAMAGE){
				if(e->flag==EF_BOMB || !(e->flag&EF_DAMAGE)) continue;
				else{
					BossDamageApply(*b, *e, damage);

					//return TRUE;
				}
			}
		}
	}
//	return FALSE;
}


static int PutBoss(int x, int y, uint32_t id)
{
	//if(EnemyNow+1>=ENEMY_MAX) return;
	//e = Enemy+ (*(EnemyInd+EnemyNow));
	//EnemyNow++;
	//n      = 4 + (((BYTE)p[4])<<2);
	//e->x   = (*(short *)(&p[0]));	//((int)(*(short *)(&p[0])))*64;
	//e->y   = (*(short *)(&p[2]));	//((int)(*(short *)(&p[2])))*64;

	auto it = std::ranges::find_if(Boss, [](const auto& it) {
		return !it.IsUsed;
	});

	// まず、あり得ないのだが... //
	if(it == std::end(Boss)) {
		return BOSS_MAX;
	}

	auto *e = &it->Edat;

	const uint32_t addr = (4 + (id << 2)); // ちと、やばいね...
	InitEnemyDataX64(e,x,y,addr);
	e->item = 0;

	/*
	e->x = x;
	e->y = y;

	// ランダム配置に対応するぞ //
	//e->x = (e->x==X_RNDV) ? GX_RND : (e->x<<6);
	//e->y = (e->y==Y_RNDV) ? GY_RND : (e->y<<6);
	addr   = 4 + (id<<2);		// ちと、やばいね...
	e->cmd = (*(DWORD *)(&ECL_Head[addr]));

	e->call_addr = e->cmd;

	e->hp       = 0xffffffff;
	e->amp      = 0;
	e->anm_ptn  = 0;
	e->anm_sp   = 0;
	e->anm_c    = 0;
	e->count    = 0;
	e->evscore  = 0;
	e->d        = 64;
	e->flag     = EF_DAMAGE|EF_DRAW|EF_HITSB;

	e->tama_c   = rnd()&0xff;
	e->t_rep    = 0;			// 弾の発射間隔(０：自動発射しない)
	e->g_width  = 0;
	e->g_height = 0;

	e->item     = 0;

	e->rep_c    = 0;
	e->cmd_c    = 0;
	e->v        = 64;
	e->vd       = 0;
	e->vx       = cosl(e->d,e->v);
	e->vy       = sinl(e->d,e->v);

	e->LLaserRef = 0;

	e->t_cmd.c      = 0;
	e->t_cmd.cmd    = TC_WAY;
	e->t_cmd.d      = 64;
	e->t_cmd.n      = 1;
	e->t_cmd.option = TE_NONE;
	e->t_cmd.type   = T_NORM;
	e->t_cmd.v      = 3;
	e->t_cmd.x      = 0;
	e->t_cmd.y      = 0;

	e->t_cmd.dw     = 16;
	e->t_cmd.ns     = 1;
	e->t_cmd.rep    = 0;
	e->t_cmd.vd     = 0;

	// 変数用レジスタの初期化 //
	e->GR[0] = e->GR[1] = e->GR[2] = e->GR[3] = 0;
	e->GR[4] = e->GR[5] = e->GR[6] = e->GR[7] = 0;

	// 割り込みベクタの初期化 //
	InitECLInterrupt(e);
*/
	return std::distance(Boss, it);
}

// ノーマルECL互換の移動 //
static void BossSTDMove(BOSS_DATA *b)
{
	ENEMY_DATA	*e = &(b->Edat);

	// 通常の敵の処理 //
	CheckECLInterrupt(e);
	parse_ECL(e);

	// 弾発射モードによる分岐 //
	if(e->t_rep){
		e->tama_c = (e->tama_c+1)%(e->t_rep);
		if(e->tama_c==0){
			TamaCmd = e->t_cmd;
			TamaCmd.x += e->x;
			TamaCmd.y += e->y;
			tama_set();
		}
	}

	switch(b->ExState){
		case(BEXST_WING01):
			if(b->ExCount < 64+16+8) b->ExCount++;
		break;
	}
}

// ボスの体力の総和を求める //
uint32_t GetBossHPSum(void)
{
	uint32_t	HP_Sum = 0;
	ENEMY_DATA		*e;

	for(auto& it : Boss) {
		auto *b = &it;
		if(b->IsUsed){
			e = &(b->Edat);
			HP_Sum += e->hp;
		}
	}

	return HP_Sum;
}

// ボス用割り込み処理 //
void BossINT(ENEMY_DATA *e, uint8_t IntID)
{
	auto b = std::ranges::find_if(Boss, [e](const auto& b) {
		return ((&b.Edat) == e);
	});
	if(b == std::end(Boss)) {
		return;
	}

	// 割り込み番号による分岐 //
	switch(IntID){
		case(ECLINT_SNAKEON):
			SnakySet(b,30,11);
		break;

		case(ECLINT_LBWING01):	// 蝶の羽も描画する
			b->ExState = BEXST_WING01;
			b->ExCount = 0;
		break;

		case(ECLINT_LBWING02):	// 鳥の羽も描画する
			b->ExState = BEXST_WING02;
			b->ExCount = 0;
		break;

		case(ECLINT_BITON5):
			BitSet(b, 5, 3);
		break;

		case(ECLINT_BITON6):
			BitSet(b, 6, 3);
		break;

		case(ECLINT_SHILD1):
			b->ExState = BEXST_SHILD1;
		break;

		case(ECLINT_SHILD2):
			b->ExState = BEXST_SHILD2;
		break;
	}
}


// ビット攻撃アドレス指定 //
void BossBitAttack(ENEMY_DATA *e, uint32_t AtkID)
{
	const auto* b = std::ranges::find_if(Boss, [e](const auto& b) {
		return ((&b.Edat) == e);
	});
	if(b == std::end(Boss)) {
		return;
	}

	BitSelectAttack(AtkID);
}

// ビットにレーザーコマンドセット //
void BossBitLaser(ENEMY_DATA *e, uint8_t cmd)
{
	const auto* b = std::ranges::find_if(Boss, [e](const auto& b) {
		return ((&b.Edat) == e);
	});
	if(b == std::end(Boss)) {
		return;
	}

	BitLaserCommand(cmd);
}

// ビット命令送信 //
void BossBitCommand(ENEMY_DATA *e, uint8_t Cmd, int Param)
{
	const auto* b = std::ranges::find_if(Boss, [e](const auto& b) {
		return ((&b.Edat) == e);
	});
	if(b == std::end(Boss)) {
		return;
	}

	BitSendCommand(Cmd, Param);
}


// 残りビット数を返す //
int BossGetBitLeft(void)
{
	return BitGetNum();
}
