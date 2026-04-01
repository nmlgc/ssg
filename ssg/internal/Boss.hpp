/*
 *   Boss class
 *
 */

#pragma once

#include "BOSS.H"

class C_BOSS {
private:
	// 現在のボスの数
	uint16_t BossNow;

	// ボスデータ格納用構造体
	BOSS_DATA Boss[BOSS_MAX];

	// ノーマルECL互換の移動
	void BossSTDMove(BOSS_DATA *b);

	bool BossDamageApply(BOSS_DATA& b, ENEMY_DATA& e, int damage);

public:
	// ボスデータ配列を初期化する(中断、ステージクリア時に使用)
	void Init(void);

	// ボスをセットする
	void Set(int x, int y, uint32_t BossID);

	// ボスを動かす
	// Returns the HP sum across all bosses.
	uint32_t Move(void);

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


extern C_BOSS Boss;
