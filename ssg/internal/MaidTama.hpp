/*                                                                           */
/*   MaidTama.h   メイドさんなショットの処理                                 */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_MAIDTAMA_H
#define PBGWIN_MAIDTAMA_H		"めいどたま : Version 0.01 : Update 2000/02/25"
//#pragma message(PBGWIN_MAIDTAMA_H)

#include "hatoyama/api/export.h"
#include "ssg/Input.h"
#include "ssg/internal/Tama.hpp"

class C_ENEMY;
class C_FRAGMENT;
class C_PLAYRANK;
class C_VIV;
struct HOOKS;
struct MAID;



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

using MAIDTAMA_DATA_CSPAN = std::span<const TAMA_DATA, MAIDTAMA_MAX>;



class C_MAIDTAMA {
private:
	HOOKS& Hooks;
	C_ENEMY& Enemy;
	C_FRAGMENT& Fragment;
	C_PLAYRANK& PlayRank;
	C_TAMA& Tama;
	TAMA_CMD& TamaCmd;
	C_VIV& Viv;

	///// [ 変数 ] /////

	// 現在の数
	uint16_t MaidTamaNow;

	// 自機ショットの格納用構造体
	std::array<TAMA_DATA, MAIDTAMA_MAX> MaidTama;

	// 弾の順番を維持するための配列(TAMA.CPP互換)
	std::array<uint16_t, MAIDTAMA_MAX> MaidTamaInd;

	//// 弾コマンド用マクロ ////
	void TamaSTDForm(uint8_t c);
	void TamaSetHoming(uint8_t c);
	void TamaSetDeg(uint8_t d, uint8_t dw);
	void TamaSetNum(uint8_t n, uint8_t ns);
	void TamaSetSpd(uint8_t v, char a);
	void TamaSetXY(int x, int y);

	void MTamaSet(void);
	void MLaserSet(uint16_t time);

	///// [ひみつの関数] /////
	void SetT_A0(void); // めいどたまＴＹＰＥ－Ａ
	void SetT_A1(void);
	void SetT_A2(void);
	void SetT_A3(void);
	void SetT_A4(void);
	void SetT_A5(void);
	void SetT_A6(void);
	void SetT_A7(void);
	void SetT_A8(void);

	void SetT_B0(void); // めいどたまＴＹＰＥ－Ｂ
	void SetT_B1(void);
	void SetT_B2(void);
	void SetT_B3(void);
	void SetT_B4(void);
	void SetT_B5(void);
	void SetT_B6(void);
	void SetT_B7(void);
	void SetT_B8(void);

	void SetT_C0(void); // めいどたまＴＹＰＥ－Ｃ
	void SetT_C1(void);
	void SetT_C2(void);
	void SetT_C3(void);
	void SetT_C4(void);
	void SetT_C5(void);
	void SetT_C6(void);
	void SetT_C7(void);
	void SetT_C8(void);

	void SetT_D0(void); // めいどたまＴＹＰＥ－Ｄ
	void SetT_D1(void);
	void SetT_D2(void);
	void SetT_D3(void);
	void SetT_D4(void);
	void SetT_D5(void);
	void SetT_D6(void);
	void SetT_D7(void);
	void SetT_D8(void);

	void SetWideBomb(void);
	void SetHomingBomb(void);
	void SetLaserBomb(void);
	void SetCactusBomb(void);

	static constexpr void (C_MAIDTAMA::*MaidTamaFunc[4][9])(void) = {
		{
			&C_MAIDTAMA::SetT_A0, &C_MAIDTAMA::SetT_A1, &C_MAIDTAMA::SetT_A2,
			&C_MAIDTAMA::SetT_A3, &C_MAIDTAMA::SetT_A4, &C_MAIDTAMA::SetT_A5,
			&C_MAIDTAMA::SetT_A6, &C_MAIDTAMA::SetT_A7, &C_MAIDTAMA::SetT_A8,
		}, {
			&C_MAIDTAMA::SetT_B0, &C_MAIDTAMA::SetT_B1, &C_MAIDTAMA::SetT_B2,
			&C_MAIDTAMA::SetT_B3, &C_MAIDTAMA::SetT_B4, &C_MAIDTAMA::SetT_B5,
			&C_MAIDTAMA::SetT_B6, &C_MAIDTAMA::SetT_B7, &C_MAIDTAMA::SetT_B8
		}, {
			&C_MAIDTAMA::SetT_C0, &C_MAIDTAMA::SetT_C1, &C_MAIDTAMA::SetT_C2,
			&C_MAIDTAMA::SetT_C3, &C_MAIDTAMA::SetT_C4, &C_MAIDTAMA::SetT_C5,
			&C_MAIDTAMA::SetT_C6, &C_MAIDTAMA::SetT_C7, &C_MAIDTAMA::SetT_C8
		}, {
			&C_MAIDTAMA::SetT_D0, &C_MAIDTAMA::SetT_D1, &C_MAIDTAMA::SetT_D2,
			&C_MAIDTAMA::SetT_D3, &C_MAIDTAMA::SetT_D4, &C_MAIDTAMA::SetT_D5,
			&C_MAIDTAMA::SetT_D6, &C_MAIDTAMA::SetT_D7, &C_MAIDTAMA::SetT_D8
		},
	};

	static constexpr void (C_MAIDTAMA::*MaidBombFunc[4])(void) = {
		&C_MAIDTAMA::SetWideBomb,
		&C_MAIDTAMA::SetHomingBomb,
		&C_MAIDTAMA::SetLaserBomb,
		&C_MAIDTAMA::SetCactusBomb
	};

public:
	C_MAIDTAMA(
		HOOKS& Hooks,
		C_ENEMY& Enemy,
		C_FRAGMENT& Fragment,
		C_PLAYRANK& PlayRank,
		C_TAMA& Tama,
		C_VIV& Viv
	) noexcept :
		Hooks(Hooks),
		Enemy(Enemy),
		Fragment(Fragment),
		PlayRank(PlayRank),
		Tama(Tama),
		TamaCmd(Tama.TamaCmd),
		Viv(Viv) {
	}

	///// [ 関数 ] /////

	// たま発射！！
	void Set(INPUT_BITS, bool in_msg);

	// 弾移動＆ヒットチェック
	HATOYAMA_API void Move(void);

	// 弾ハッシュテーブル初期化
	HATOYAMA_API void IndSet(void);

	MAIDTAMA_DATA_CSPAN Data(void) const {
		return MaidTama;
	}

	const std::span<const uint16_t> Inds(void) const {
		return std::span(MaidTamaInd).first(MaidTamaNow);
	}
};



#endif
