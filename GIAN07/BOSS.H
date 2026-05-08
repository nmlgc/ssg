/*                                                                           */
/*   Boss.h   ボスの処理(中ボス含む)                                         */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_BOSS_H
#define PBGWIN_BOSS_H		"BOSS : Version 0.01 : Update 2000/02/27"
//#pragma message(PBGWIN_BOSS_H)

#include "ENEMY.H"
#include "MAID.H"

struct HOOKS;
class C_BIT;
class C_BOMBEFC;
class C_FRAGMENT;
class C_LASER;
class C_SEFFECT;
class C_SNAKY;


///// [ 定数 ] /////

// 最大数 //
constexpr auto BOSS_MAX = 4; // ボスの最大出現数(二匹以上出るのか？？)

// ボスの状態 //
#define BEXST_NORM   0x00 // 通常のＥＣＬで動作中
#define BEXST_DEAD   0x01 // 死亡中<-こいつは多分使っていないぞ(2000/10/31)
#define BEXST_WING01 0x02 // 蝶の羽
#define BEXST_WING02 0x03 // 天使の羽
#define BEXST_SHILD1 0x04 // シールド１
#define BEXST_SHILD2 0x05 // シールド２


///// [構造体] /////

// ボスデータ //
struct BOSS_DATA {
	ENEMY_DATA		Edat;					// 標準の敵データ(実体であることに注意)

	uint32_t	ExCount;	// ある状態におけるカウンタ(推移時にゼロ初期化)
	uint8_t	ExState;	// 特殊状態
	uint8_t	IsUsed;	// このデータは使用されているか(非ゼロなら使用されている)
};

using BOSS_DATA_CSPAN = std::span<const BOSS_DATA, BOSS_MAX>;



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
	void STDMove(BOSS_DATA *b);

	bool DamageApply(BOSS_DATA& b, ENEMY_DATA& e, int damage);

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

#endif
