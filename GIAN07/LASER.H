/*************************************************************************************************/
/*   LASER.H   レーザーに関する処理(反射,ショート)                                               */
/*                                                                                               */
/*************************************************************************************************/

#ifndef PBGWIN_LASER_H
#define PBGWIN_LASER_H		"LASEER : Ver 0.51 : Update 2000/02/17"
//#pragma message(PBGWIN_LASER_H)

import std.compat;
#include "hatoyama/logic/coords.h"

class C_LLASER;
class C_PLAYRANK;
class C_RNG;
class C_VIV;

///// [更新履歴] /////

// 2000/02/17 : 新しいシステムに移行開始。無限遠レーザーと完全に分離

/*-> ここからはちょっと古いよ(1999..)
 * (4/3)  10:36 開発開始
 * (4/6)  12:00 ついにポリゴン＆クリッピング関数が完成。描画はいつ出来るのか？
 * (4/7)  12:02 全てのレーザーを同じ構造体で扱う事にした
 * (4/8)   7:23 無限遠レーザーの制作
 * (4/9)   2:01 反射レーザーを一応打ち込み終わる
 * (4/9)   2:59 反射レーザー完成
 * (4/11) 14:05 ショート＆反射レーザーの当たり判定完成
 * (4/11) 15:17 リフレクターのヒットチェックを強化(バグは消えたが遅くなった)
 *
 * (9/23) 16:18 ライン描画、ＥＣＬ対応などが完了
 */


////レーザー定数////
#define LASER_MAX			1000			// レーザーの最大発生本数


////レーザーの種類定数////
#define LS_SHORT 0x00 // ショートレーザー
#define LS_REF   0x01 // 反射レーザー
#define LS_LONG  0x02 // 無限遠レーザー
#define LS_LONGY 0x03 // 縦無限遠レーザー(角度指定は無効 64固定)


////レーザーフラグ定数(一部レーザーの種類に依存します)////
#define LF_NONE  0x00 // フラグ無し状態
#define LF_CLEAR 0x01 // レーザーが消滅中である

#define LF_SHOT  0x02 // レーザー発射中
#define LF_HIT   0x04 // レーザーヒット中(REF_OBJECTに対して)
#define LF_NMOVE 0x06 // レーザーの長さ変わらず(LF_SHOT|LF_HIT)

#define LF_DELETE 0x80 // レーザーを消去する(処理対象から外す)


////レーザー発動コマンド構造体////
struct LASER_CMD {
	int  x,y;		// 始点の座標
	int  v;			// レーザーの初速度

	int w;			// レーザーの太さ        (x64座標を使用する)
	int l;			// レーザーの長さ最終値  (x64座標を使用する)
	int l2;			// レーザーの発射位置補正(x64...)

	uint8_t d;	// 発射角
	uint8_t dw;	// 発射幅

	uint8_t n;	// レーザーの本数
	uint8_t c;	// レーザーの色

	char a;			// 加速度(つかうのかな???)
	uint8_t cmd;	// レーザー発動コマンド(ほとんど弾と同じかも)
	uint8_t type;	// ショート、無限遠など
	uint8_t notr;	// 反射しないリフレクターの番号
};


////レーザー用構造体////
struct LASER_DATA {
	int x,y;	// 現在の始点
	int vx,vy;	// 速度の(X,Y)成分
	int lx,ly;	// 表示座標の加算値(長さ)
	int wx,wy;	// 表示座標の加算値(太さ)
	int v;	// 速度

	WINDOW_POINT p[4]; // 表示する座標

	char a;	// 加速度(つかうのか??)
	uint8_t d;	// 進行方向

	int w,wmax;	// 太さ
	int l,lmax;	// 現在の長さ、長さの最終値
	int ltemp;	// 反射レーザー専用変数(発射＆ヒットの場合にのみ使用)

	uint16_t count;	// フレームカウンタ
	uint8_t c;	// 色
	uint8_t type;	// 種類
	uint8_t flag;	// 消去要請フラグ等(エフェクト含む)
	uint8_t notr;	// 反射しないリフレクターの番号
	uint8_t evade;	// かすり用フラグ
};

using LASER_DATA_CSPAN = std::span<const LASER_DATA, LASER_MAX>;


class C_LASER {
public:
	// 標準レーザーコマンド構造体
	LASER_CMD LaserCmd;

private:
	C_LLASER const& LLaser;
	C_PLAYRANK const& PlayRank;
	C_RNG& RNG;
	C_VIV& Viv;

	// レーザーの本数
	uint16_t LaserNow;

	// レーザー格納用構造体
	std::array<LASER_DATA, LASER_MAX> Laser;

	// レーザー順番維持用配列
	std::array<uint16_t, LASER_MAX> LaserInd;

	// レーザーの進行方向をセットする
	uint8_t laser_dir(uint16_t i);

	// 種類により分岐し座標を更新する
	void Lmove(LASER_DATA *lp);

	// レーザーの当たり判定
	void laser_hitchk(LASER_DATA *lp);

	// 反射レーザーの移動
	void REFL_move(LASER_DATA *lp);

	// ﾘﾌﾚｸﾀｰとの当たり判定(ﾋｯﾄ->非０)
	int REFL_hit(const LASER_DATA *lp);

public:
	C_LASER(
		C_LLASER const& LLaser,
		C_PLAYRANK const& PlayRank,
		C_RNG& RNG,
		C_VIV& Viv
	) noexcept : LLaser(LLaser), PlayRank(PlayRank), RNG(RNG), Viv(Viv) {
	}

	// レーザーをセットする(難易度変更"有り")
	void Set(void);

	// レーザーをセットする(難易度変更"無し")
	void SetEX(void);

	// レーザーを動かす
	void Move(void);

	// レーザー全てに消去エフェクトをかける
	void Clear(void);

	// レーザー順序用配列の初期化
	void IndSet(void);

	const LASER_DATA_CSPAN Data(void) const noexcept {
		return Laser;
	}

	const std::span<const uint16_t> Inds(void) const {
		return std::span(LaserInd).first(LaserNow);
	}
};

#endif
