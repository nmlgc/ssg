/*                                                                           */
/*   Boss.h   ボスの処理(中ボス含む)                                         */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_BOSS_H
#define PBGWIN_BOSS_H		"BOSS : Version 0.01 : Update 2000/02/27"
//#pragma message(PBGWIN_BOSS_H)

#include "ENEMY.H"
#include "MAID.H"


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



///// [ 関数 ] /////
void BossDataInit(void);	// ボスデータ配列を初期化する(中断、ステージクリア時に使用)

// ボスをセットする
void BossSet(int x, int y, uint32_t BossID);

// ボスを動かす
// Returns the HP sum across all bosses.
uint32_t BossMove(void);

// ボスを描画する
void BossDraw(BOSS_DATA_CSPAN, const MAID&);

void BossClearCmd(void);	// ボス用・敵弾クリアの前処理関数
int  BossGetBitLeft(void);	// 残りビット数を返す

void BossKillAll(void);	// 現在出現しているボス全てのＨＰを０にする
bool BossDamage(int x, int y, int damage);	// ボスにダメージを与える
bool BossDamage2(int x, int y, int damage);	// ボスにダメージを与える(ｙ上方向無限Ver)
void BossDamage3(int x, int y, uint8_t d);	// ボスにダメージを与える(ナナメレーザー)
void BossDamage4(int damage);	// ボスにダメージを与える(すべての敵)

uint32_t GetBossHPSum(void);	// ボスの体力の総和を求める

void BossINT(ENEMY_DATA *e, uint8_t IntID);	// ボス用割り込み処理
void BossBitAttack(ENEMY_DATA *e, uint32_t AtkID);	// ビット攻撃アドレス指定
void BossBitLaser(ENEMY_DATA *e, uint8_t LaserCmd);	// ビットにレーザーコマンドセット
void BossBitCommand(ENEMY_DATA *e, uint8_t Cmd, int Param);	// ビット命令送信


///// [ 変数 ] /////
extern BOSS_DATA	Boss[BOSS_MAX];	// ボスデータ格納用構造体
extern uint16_t	BossNow;	// 現在のボスの数

#endif
