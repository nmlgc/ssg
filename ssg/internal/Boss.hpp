/*
 *   Boss class
 *
 */

#pragma once

#include "ssg/Boss.h"

struct HOOKS;
class C_BIT;
class C_BOMBEFC;
class C_ENEMY;
class C_FRAGMENT;
class C_ITEM;
class C_LASER;
class C_SEFFECT;
class C_SNAKY;
class C_TAMA;
class C_VIV;

class C_BOSS {
private:
	HOOKS& Hooks;
	C_BIT& Bit;
	C_BOMBEFC& BombEfc;
	C_ENEMY& Enemy;
	C_FRAGMENT& Fragment;
	C_ITEM& Item;
	C_LASER& Laser;
	C_SEFFECT& SEffect;
	C_SNAKY& Snaky;
	C_TAMA& Tama;
	C_VIV& Viv;

	// 現在のボスの数
	uint16_t BossNow;

	// ボスデータ格納用構造体
	BOSS_DATA Boss[BOSS_MAX];

	// ノーマルECL互換の移動
	void BossSTDMove(BOSS_DATA *b);

	bool BossDamageApply(BOSS_DATA& b, ENEMY_DATA& e, int damage);

public:
	C_BOSS(
		HOOKS& Hooks,
		C_BIT& Bit,
		C_BOMBEFC& BombEfc,
		C_ENEMY& Enemy,
		C_FRAGMENT& Fragment,
		C_ITEM& Item,
		C_LASER& Laser,
		C_SEFFECT& SEffect,
		C_SNAKY& Snaky,
		C_TAMA& Tama,
		C_VIV& Viv
	) noexcept :
		Hooks(Hooks),
		Bit(Bit),
		BombEfc(BombEfc),
		Enemy(Enemy),
		Fragment(Fragment),
		Laser(Laser),
		Item(Item),
		SEffect(SEffect),
		Snaky(Snaky),
		Tama(Tama),
		Viv(Viv) {
	}

	// ボスデータ配列を初期化する(中断、ステージクリア時に使用)
	void Init(void);

	// ボスをセットする
	void Set(int x, int y, uint32_t BossID);

	// ボスを動かす
	void Move(void);

	// ボス用・敵弾クリアの前処理関数
	void ClearCmd(void);

	// 残りビット数を返す
	int GetBitLeft(void) const;

	// 現在出現しているボス全てのＨＰを０にする
	void KillAll(void);

	// ボスにダメージを与える
	bool Damage(int x, int y, int damage);

	// ボスにダメージを与える(ｙ上方向無限Ver)
	bool Damage2(int x, int y, int damage);

	// ボスにダメージを与える(ナナメレーザー)
	void Damage3(int x, int y, uint8_t d);

	// ボスにダメージを与える(すべての敵)
	void Damage4(int damage);

	// ボスの体力の総和を求める
	uint32_t GetHPSum(void) const;

	// ボス用割り込み処理
	void INT(ENEMY_DATA *e, uint8_t IntID);

	// ビット攻撃アドレス指定
	void BitAttack(ENEMY_DATA *e, uint32_t AtkID);

	// ビットにレーザーコマンドセット
	void BitLaser(ENEMY_DATA *e, uint8_t LaserCmd);

	// ビット命令送信
	void BitCommand(ENEMY_DATA *e, uint8_t Cmd, int Param);

	decltype(BossNow) NumAlive(void) const {
		return BossNow;
	}

	const BOSS_DATA_CSPAN Data(void) const {
		return Boss;
	}
};
