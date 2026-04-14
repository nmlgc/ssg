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
#include "GIAN.H"
#include "ITEM.H"
#include "LLASER.H"
#include "SCROLL.H"
#include "ssg/Sound.h"
#include "hatoyama/engine/snd.h"

///// [ 変数 ] /////

// 公開グローバル //
C_BOSS Boss;


// ボスデータ配列を初期化する(中断、ステージクリア時に使用) //
void C_BOSS::Init(void)
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
void C_BOSS::Set(int x, int y, uint32_t BossID)
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

	// ｘ６４座標に変換してボスをセットする //
	Enemy.InitEnemyDataX64(e, PixelToWorld(x), PixelToWorld(y), BossID);
	e->item = 0;

	it->ExCount = 0;
	it->ExState = BEXST_NORM;
	it->IsUsed  = 0xff;

	Enemy.ECL_Parse(e);
	//ObjectLockOn(&(it->Edat.x),&(it->Edat.y),it->Edat.g_width,it->Edat.g_height);

	BossNow++;
}


// ボスを動かす //
uint32_t C_BOSS::Move(void)
{
	uint32_t HP_Sum = 0;
	ENEMY_DATA		*e;

	Tama.ResetHoming();

	for(auto& it : Boss) {
		auto *b = &it;
		if(b->IsUsed){
			e = &(b->Edat);
			e->IsDamaged = 0;
			this->STDMove(b);

			// サボテンヒットチェック //
			if(HITCHK(e->x,Viv.x,e->g_width) && HITCHK(e->y,Viv.y,e->g_height) && Viv.muteki==0){
				// ここら辺で敵にダメージを与えるとおもしろいかも？ //
				if(e->flag & EF_HITSB) {
					Viv.Dead();
				}
			}

			// ホーミングの準備 //
			if(e->flag & EF_DAMAGE) {
				Tama.UpdateHoming({ &e->x, &e->y });
			}

			// 体力の総和を表示する //
			HP_Sum += b->Edat.hp;

			// アニメーションの動作 //
			Enemy.AnimeMove(e);

			e->count++;
		}
	}
	return HP_Sum;
}


// ボス用・敵弾クリアの前処理関数 //
void C_BOSS::ClearCmd(void)
{
	Bit.Delete();
}


// 現在出現しているボス全てのＨＰを０にする //
void C_BOSS::KillAll(void)
{
	// 破壊後の破片放出などの処理は、ダメージを与えて破壊するのと同等の関数を //
	// 使用するが、当然のごとく、得点＆経験値？は入手できない                 //
	// レーザークローズも忘れずに！！                                         //

	ENEMY_DATA		*e;

	for(auto& it : Boss) {
		auto *b = &it;
		if(b->IsUsed){
			e = &(b->Edat);
			Snaky.Delete(b);
			Fragment.Set(e->x,e->y,FRG_FATCIRCLE);
			BombEfc.Set(e->x, e->y, EXBOMB_STD);
			Snd_SEPlay(SOUND_ID_BOSSBOMB, e->x);
			Enemy.Explode(*e);
			b->IsUsed = 0;
		}
	}

	BossNow = 0;
}

bool C_BOSS::DamageApply(BOSS_DATA& b, ENEMY_DATA& e, int damage)
{
	e.IsDamaged = ((e.count) & 1);
	if(e.hp <= damage) { // ボスの死亡処理(後で変更すること!!)
		Snaky.Delete(&b);
		Bit.Delete();
		Enemy.Clear();
		Fragment.Set(e.x, e.y, FRG_FATCIRCLE);
		BombEfc.Set(e.x, e.y, EXBOMB_STD);
		ScrollCommand(SCMD_QUAKE);
		Snd_SEPlay(SOUND_ID_BOSSBOMB, e.x);
		Viv.PowerUp(Cast::down<uint8_t>(e.hp));
		Enemy.Explode(e);

		// 最後の一匹だった場合 //
		if(BossNow == 1) {
			char buf[100];
			const auto temp = Tama.ToScore(); // 弾→スコアエフェクト
			// sprintf(buf, "%3d Evade  %5dPts", Viv.evade, Viv.evadesc);
			sprintf(buf, "  Bonus    %7uPts", temp);
			SEffect.SetString(180, 60, buf);
			Viv.score_add(temp);
		}

		if(e.item) {
			Item.Set(e.x, e.y, e.item);
		}
		Viv.score_add(e.score);
		Laser.Clear();
		b.IsUsed = 0;
		BossNow--; // ボスの参照カウント？を使用する
	} else {
		Snd_SEPlay(SOUND_ID_HIT, e.x);
		Viv.PowerUp(damage);
		e.hp -= damage;
	}
	return true;
}

// ボスにダメージを与える //
bool C_BOSS::Damage(int x, int y, int damage)
{
	ENEMY_DATA		*e;

	damage -= (Bit.GetNum() >> 1);
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
					return DamageApply(*b, *e, damage);
				}
			}
		}
	}
	return false;
}

// ボスにダメージを与える(ｙ上方向無限Ver) //
bool C_BOSS::Damage2(int x, int y, int damage)
{
	ENEMY_DATA		*e;
	bool	ret_val = false;

	damage -= (Bit.GetNum() >> 1);
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
					ret_val = DamageApply(*b, *e, damage);
				}
			}
		}
	}
	return ret_val;
}

// ボスにダメージを与える(ナナメレーザー) //
void C_BOSS::Damage3(int x, int y, uint8_t d)
{
	ENEMY_DATA		*e;
	//BOOL			ret_val = FALSE;
	int				damage  = 2;

	damage -= (Bit.GetNum() >> 1);
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
					DamageApply(*b, *e, damage);
				}
			}
		}
	}
}


// ボスにダメージを与える(すべての敵)
void C_BOSS::Damage4(int damage)
{
	ENEMY_DATA		*e;

	damage -= (Bit.GetNum() >> 1);
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
					DamageApply(*b, *e, damage);

					//return TRUE;
				}
			}
		}
	}
//	return FALSE;
}


// ノーマルECL互換の移動 //
void C_BOSS::STDMove(BOSS_DATA *b)
{
	ENEMY_DATA	*e = &(b->Edat);

	// 通常の敵の処理 //
	Enemy.ECL_Parse(e);

	// 弾発射モードによる分岐 //
	if(e->t_rep){
		e->tama_c = (e->tama_c+1)%(e->t_rep);
		if(e->tama_c==0){
			auto& TamaCmd = Tama.TamaCmd;
			TamaCmd = e->t_cmd;
			TamaCmd.x += e->x;
			TamaCmd.y += e->y;
			Tama.Set();
		}
	}

	switch(b->ExState){
		case(BEXST_WING01):
			if(b->ExCount < 64+16+8) b->ExCount++;
		break;
	}
}

// ボスの体力の総和を求める //
uint32_t C_BOSS::GetHPSum(void) const
{
	uint32_t	HP_Sum = 0;

	for(auto& it : Boss) {
		auto *b = &it;
		if(b->IsUsed){
			const auto *e = &(b->Edat);
			HP_Sum += e->hp;
		}
	}

	return HP_Sum;
}

// ボス用割り込み処理 //
void C_BOSS::INT(ENEMY_DATA *e, uint8_t IntID)
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
			Snaky.Set(b, 30, 11);
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
			Bit.Set(b, 5, 3);
		break;

		case(ECLINT_BITON6):
			Bit.Set(b, 6, 3);
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
void C_BOSS::BitAttack(ENEMY_DATA *e, uint32_t AtkID)
{
	const auto* b = std::ranges::find_if(Boss, [e](const auto& b) {
		return ((&b.Edat) == e);
	});
	if(b == std::end(Boss)) {
		return;
	}

	Bit.SelectAttack(AtkID);
}

// ビットにレーザーコマンドセット //
void C_BOSS::BitLaser(ENEMY_DATA *e, uint8_t cmd)
{
	const auto* b = std::ranges::find_if(Boss, [e](const auto& b) {
		return ((&b.Edat) == e);
	});
	if(b == std::end(Boss)) {
		return;
	}

	Bit.LaserCommand(cmd);
}

// ビット命令送信 //
void C_BOSS::BitCommand(ENEMY_DATA *e, uint8_t Cmd, int Param)
{
	const auto* b = std::ranges::find_if(Boss, [e](const auto& b) {
		return ((&b.Edat) == e);
	});
	if(b == std::end(Boss)) {
		return;
	}

	Bit.SendCommand(Cmd, Param);
}


// 残りビット数を返す //
int C_BOSS::GetBitLeft(void) const
{
	return Bit.GetNum();
}
