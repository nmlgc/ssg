/*                                                                           */
/*   MaidTama.h   メイドさんなショットの処理                                 */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_MAIDTAMA_H
#define PBGWIN_MAIDTAMA_H		"めいどたま : Version 0.01 : Update 2000/02/25"
//#pragma message(PBGWIN_MAIDTAMA_H)

#ifdef __cplusplus
import std.compat;
#endif

typedef struct TAMA_DATA TAMA_DATA;



///// [ 定数 ] /////

// 最大値 //
#define MAIDTAMA_MAX		200		// 自機ショットの最大数

#define TID_WIDE_MAIN		0x00	// ワイド・メインショットのＩＤ
#define TID_WIDE_SUB		0x01	// ワイド・サブショットのＩＤ
#define TID_HOMING_MAIN		0x02	// ホーミング・メインショットのＩＤ
#define TID_HOMING_SUB		0x03	// ホーミング・サブショットのＩＤ
#define TID_LASER_MAIN		0x04	// レーザー・メインショット？？のＩＤ
#define TID_LASER_SUB		0x05	// レーザー・サブショットのＩＤ

#define TID_HOMING_BOMB_A	0x06	// ホーミング用ボム(移動中)
#define TID_HOMING_BOMB_B	0x07	// ホーミング用ボム(誘爆中)

#define TDM_WIDE_MAIN		6		// ワイド・メインショットのダメージ
#define TDM_WIDE_SUB		4		// ワイド・サブショットのダメージ
#define TDM_HOMING_MAIN		6		// ホーミング・メインショットのダメージ
#define TDM_HOMING_SUB		7		// ホーミング・サブショットのダメージ
#define TDM_LASER_MAIN		2		// レーザー・メインショットのダメージ
#define TDM_LASER_SUB		5		// レーザー・サブショットのダメージ


#ifdef __cplusplus
using MAIDTAMA_DATA_CSPAN = std::span<const TAMA_DATA, MAIDTAMA_MAX>;
#endif


#endif
