/*                                                                           */
/*   Boss.h   ボスの処理(中ボス含む)                                         */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_BOSS_H
#define PBGWIN_BOSS_H		"BOSS : Version 0.01 : Update 2000/02/27"
//#pragma message(PBGWIN_BOSS_H)

#include "ssg/Enemy.h"

///// [ 定数 ] /////

// 最大数 //
#define BOSS_MAX	4	// ボスの最大出現数(二匹以上出るのか？？)

// ボスの状態 //
#define BEXST_NORM   0x00 // 通常のＥＣＬで動作中
#define BEXST_DEAD   0x01 // 死亡中<-こいつは多分使っていないぞ(2000/10/31)
#define BEXST_WING01 0x02 // 蝶の羽
#define BEXST_WING02 0x03 // 天使の羽
#define BEXST_SHILD1 0x04 // シールド１
#define BEXST_SHILD2 0x05 // シールド２


///// [構造体] /////

// ボスデータ //
typedef struct tagBOSS_DATA{
	ENEMY_DATA		Edat;					// 標準の敵データ(実体であることに注意)

	uint32_t	ExCount;	// ある状態におけるカウンタ(推移時にゼロ初期化)
	uint8_t	ExState;	// 特殊状態
	uint8_t	IsUsed;	// このデータは使用されているか(非ゼロなら使用されている)
} BOSS_DATA;


#ifdef __cplusplus
using BOSS_DATA_CSPAN = std::span<const BOSS_DATA, BOSS_MAX>;
#endif


#endif
