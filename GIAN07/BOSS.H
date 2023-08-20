/*                                                                           */
/*   Boss.h   ボスの処理(中ボス含む)                                         */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_BOSS_H
#define PBGWIN_BOSS_H		"BOSS : Version 0.01 : Update 2000/02/27"
//#pragma message(PBGWIN_BOSS_H)

#include "ENEMY.H"


///// [構造体] /////

// 特殊当たり判定 //
typedef struct tagEXHITCHK{
	uint8_t	flags[60][60];
} EXHITCHK;

// ボスデータ //
typedef struct tagBOSS_DATA{
	ENEMY_DATA		Edat;					// 標準の敵データ(実体であることに注意)
	EXHITCHK		*Hit;					// 特殊当たり判定(NULL なら使用しない)

	void(*ExMove)(tagBOSS_DATA *);	// 特殊移動用関数

	uint32_t	ExCount;	// ある状態におけるカウンタ(推移時にゼロ初期化)
	uint8_t	ExState;	// 特殊状態
	uint8_t	IsUsed;	// このデータは使用されているか(非ゼロなら使用されている)
} BOSS_DATA;



///// [ 関数 ] /////
void BossDataInit(void);	// ボスデータ配列を初期化する(中断、ステージクリア時に使用)
void BossSet(int x, int y, uint32_t BossID);	// ボスをセットする(For SCL)
void BossSetEx(int x, int y, uint32_t BossID);	// ボスをセットする(For ECL)
void BossMove(void);	// ボスを動かす
void BossDraw(void);	// ボスを描画する

void BossClearCmd(void);	// ボス用・敵弾クリアの前処理関数
int  BossGetBitLeft(void);	// 残りビット数を返す

void BossKillAll(void);	// 現在出現しているボス全てのＨＰを０にする
bool BossDamage(int x, int y, int damage);	// ボスにダメージを与える
bool BossDamage2(int x, int y, int damage);	// ボスにダメージを与える(ｙ上方向無限Ver)
void BossDamage3(int x, int y, uint8_t d);	// ボスにダメージを与える(ナナメレーザー)
void BossDamage4(int damage);	// ボスにダメージを与える(すべての敵)

void BossHPG_Draw(void);	// ボスの体力ゲージを描画する

uint32_t GetBossHPSum(void);	// ボスの体力の総和を求める

void BossINT(ENEMY_DATA *e, uint8_t IntID);	// ボス用割り込み処理
void BossBitAttack(ENEMY_DATA *e, uint32_t AtkID);	// ビット攻撃アドレス指定
void BossBitLaser(ENEMY_DATA *e, uint8_t LaserCmd);	// ビットにレーザーコマンドセット
void BossBitCommand(ENEMY_DATA *e, uint8_t Cmd, int Param);	// ビット命令送信


///// [ 変数 ] /////
extern uint16_t	BossNow;	// 現在のボスの数

#endif
