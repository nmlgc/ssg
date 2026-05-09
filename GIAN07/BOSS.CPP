/*                                                                           */
/*   Boss.cpp   ボスの処理(中ボス含む)                                       */
/*                                                                           */
/*                                                                           */

#include <printf/printf.h>

#include "BOSS.H"
#include "BOMBEFC.H" // 爆発エフェクト処理
#include "EFFECT.H"
#include "EnemyExCtrl.h"
#include "FRAGMENT.H"
#include "GEOMETRY.H"
#include "GIAN.H"
#include "ITEM.H"
#include "LLASER.H"
#include "MAID.H"
#include "SCROLL.H"
#include "ssg/Sound.h"
#include "hatoyama/engine/graphics_backend.h"
#include "hatoyama/engine/snd.h"
#include "hatoyama/logic/cast.h"
#include "hatoyama/logic/ut_math.h"


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



///// [ 変数 ] /////

// 公開グローバル //
BOSS_DATA		Boss[BOSS_MAX];		// ボスデータ格納用構造体
uint16_t	BossNow;	// 現在のボスの数


// 秘密の関数 //
static void BossSTDMove(BOSS_DATA *b);	// ノーマルECL互換の移動



// ボスデータ配列を初期化する(中断、ステージクリア時に使用) //
void BossDataInit(void)
{
	for(auto& it : Boss) {
		it.IsUsed   = 0;	// 要はこれをゼロにすれば良い

		// 特殊変数の初期化.... //
		it.ExCount  = 0;
		it.ExState  = BEXST_NORM;	// 特殊状態変数
	}

	// ボスの数を０にする //
	BossNow = 0;
}

// ボスをセットする //
void BossSet(int x, int y, uint32_t BossID)
{
	auto it = std::ranges::find_if(Boss, [](const auto& it) {
		return !it.IsUsed;
	});

	// まず、あり得ないのだが... //
	if(it == std::end(Boss)) {
		// ここに来たらバグ
		assert(!"Script code spawns more bosses than supported by the engine");
		return;
	}

	auto *e = &it->Edat;
	const uint32_t addr = (4 + (BossID << 2)); // ちと、やばいね...

	// ｘ６４座標に変換してボスをセットする //
	InitEnemyDataX64(e, PixelToWorld(x), PixelToWorld(y), addr);
	e->item = 0;

	it->ExCount = 0;
	it->ExState = BEXST_NORM;
	it->IsUsed  = 0xff;

	parse_ECL(e);
	//ObjectLockOn(&(it->Edat.x),&(it->Edat.y),it->Edat.g_width,it->Edat.g_height);

	BossNow++;
}


// ボスを動かす //
uint32_t BossMove(void)
{
	uint32_t HP_Sum = 0;
	ENEMY_DATA		*e;

	ResetHoming();

	for(auto& it : Boss) {
		auto *b = &it;
		if(b->IsUsed){
			e = &(b->Edat);
			e->IsDamaged = 0;
			BossSTDMove(b);

			// サボテンヒットチェック //
			if(HITCHK(e->x,Viv.x,e->g_width) && HITCHK(e->y,Viv.y,e->g_height) && Viv.muteki==0){
				// ここら辺で敵にダメージを与えるとおもしろいかも？ //
				if(e->flag&EF_HITSB) MaidDead();
			}

			// ホーミングの準備 //
			if(e->flag & EF_DAMAGE) {
				UpdateHoming({ &e->x, &e->y });
			}

			// 体力の総和を表示する //
			HP_Sum += b->Edat.hp;

			// アニメーションの動作 //
			EnemyAnimeMove(e);

			e->count++;
		}
	}
	return HP_Sum;
}

// ボスを描画する
void BossDraw(void)
{
	constexpr auto sid = SURFACE_ID::ENEMY;
	int x, y;
	int			w,h,t;
	ENEMY_DATA	*e;
	PIXEL_LTRB	wing;

	for(auto& it : Boss) {
		auto *b = &it;
		if(b->IsUsed){
			e = &(b->Edat);

			x = (e->x>>6);
			y = (e->y>>6);

			// 霊魂状態 //
			if(b->ExState == BEXST_SHILD2 && Viv.bomb_time && (e->flag&EF_DRAW)){
				wing = PIXEL_LTWH{
					(160 + (Cast::sign<int32_t>(e->count / 2) % 4) * 40),
					80,
					40,
					40
				};

				// pbg quirk: Blitted without clipping?! I'd consider this a
				// bug if it wasn't explicitly commented as such. Fine then...
				GrpBackend_SetClip(GRP_RES_RECT);

				// クリッピングなし
				GrpSurface_Blit({ (x - 20), (y - 20) }, sid, wing);

				GrpBackend_SetClip({ X_MIN, Y_MIN, (X_MAX + 1), (Y_MAX + 1) });
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
					GrpSurface_Blit({ (x - w - t), (y - h) }, sid, wing);
					wing = { 128, 176, 256, 360 };
					GrpSurface_Blit({ (x - w + t), (y - h) }, sid, wing);
				break;

				case(BEXST_WING02):
					w = 44;
					h = 52;
					wing = { 552,   0, 640, 104 };
					GrpSurface_Blit({ (x - w - 50), (y - h) }, sid, wing);
					wing = { 552, 104, 640, 208 };
					GrpSurface_Blit({ (x - w + 50), (y - h) }, sid, wing);
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
			EnemyExplode(*e);
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
		PowerUp(Cast::down<uint8_t>(e.hp));
		EnemyExplode(e);

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
