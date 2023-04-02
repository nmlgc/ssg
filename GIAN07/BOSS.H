/*                                                                           */
/*   Boss.h   ボスの処理(中ボス含む)                                         */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_BOSS_H
#define PBGWIN_BOSS_H		"BOSS : Version 0.01 : Update 2000/02/27"
//#pragma message(PBGWIN_BOSS_H)

#include "GIAN.H"
//#include "ENEMY.H"


///// [ 定数 ] /////

// 最大数 //
#define BOSS_MAX		4		// ボスの最大出現数(二匹以上出るのか？？)

// ボスの状態 //
#define BEXST_NORM			0x00		// 通常のＥＣＬで動作中
#define BEXST_DEAD			0x01		// 死亡中<-こいつは多分使っていないぞ(2000/10/31)
#define BEXST_WING01		0x02		// 蝶の羽
#define BEXST_WING02		0x03		// 天使の羽
#define BEXST_SHILD1		0x04		// シールド１
#define BEXST_SHILD2		0x05		// シールド２

// 体力ゲージ編 //
#define BOSSHPG_WIDTH		256			// 体力ゲージの幅
#define BOSSHPG_HEIGHT		24			// 体力ゲージの高さ
#define BOSSHPG_START_X		X_MAX		// 体力ゲージの初期Ｘ
#define BOSSHPG_END_X		260			// 体力ゲージの最終Ｘ

#define BHPG_DEAD			0x00		// 体力ゲージは使用されていない
#define BHPG_OPEN1			0x01		// 体力ゲージを開く(第一エフェクト中)
#define BHPG_OPEN2			0x02		// 体力ゲージを開く(体力上昇中)
#define BHPG_NORM			0x03		// 体力ゲージの準備ができている
#define BHPG_CLOSE			0x04		// 体力ゲージを閉じる
#define BHPG_OPEN3			0x05		// 体力ゲージを更新する



///// [構造体] /////

// 特殊当たり判定 //
typedef struct tagEXHITCHK{
	BYTE	flags[60][60];
} EXHITCHK;

// ボスデータ //
typedef struct tagBOSS_DATA{
	ENEMY_DATA		Edat;					// 標準の敵データ(実体であることに注意)
	EXHITCHK		*Hit;					// 特殊当たり判定(NULL なら使用しない)

	void(*ExMove)(tagBOSS_DATA *);	// 特殊移動用関数

	DWORD			ExCount;		// ある状態におけるカウンタ(推移時にゼロ初期化)
	BYTE			ExState;		// 特殊状態
	BYTE			IsUsed;			// このデータは使用されているか(非ゼロなら使用されている)
} BOSS_DATA;

// ボスの体力ゲージ //
typedef struct tagBOSSHPG_INFO{
	DWORD	Now,Max;					// 体力の現在値＆最大値
	DWORD	Next;						// 次の体力の値
	DWORD	Update;						// 更新用の値
	DWORD	Count;						// フレーム数保持

	WORD	XTemp[BOSSHPG_HEIGHT];		// ＨＰゲージの演出用
	BYTE	State;						// 状態
} BOSSHPG_INFO;



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
extern BOSS_DATA		Boss[BOSS_MAX];		// ボスデータ格納用構造体
extern WORD				BossNow;			// 現在のボスの数

#endif
