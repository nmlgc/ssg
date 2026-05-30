/*************************************************************************************************/
/*   ENEMY.H   敵の管理とか発生制御等                                                            */
/*                                                                                               */
/*************************************************************************************************/

#ifndef PBGWIN_ENEMY_H
#define PBGWIN_ENEMY_H		"ENEMY : Ver 0.41 : Update 2000/02/22"
//#pragma message(PBGWIN_ENEMY_H)

///// [ 更新履歴 ] /////

// 2000/10/17 : プレイランクの変数を参照するのを間違えていた(ConfigDat のほうを参照していた)
// 2000/03/22 : LLaser の処理を追加(実際の発射は構造体そのものに直接代入する事で行う)
// 2000/02/25 : 敵の当たり判定チェック用関数 enemy_damage() を追加
// 2000/02/22 : 敵のクリッピング範囲を変更した。

/* -> ここから下は古いよ
 * > 8/23 (20:56)  : (ox,oy) は使用しないものとする
 *                 : ECL_Head から始まるデータ <敵の数><開始アドレスx敵の数><ECLデータ>
 *
 */

#include "ssg/ECL.h"
#include "ssg/internal/Entity.hpp"
#include "ssg/internal/Laser.hpp"
#include "ssg/internal/Tama.hpp"
#include "hatoyama/logic/buffer.h"

class C_BOSS;
class C_EFFECT3D;
class C_HLASER;
class C_ITEM;
class C_LASER;
class C_TAMA;
class C_VIV;
struct C_RNG;
struct HOOKS;


//// 敵定数 ////
#define ENEMY_MAX		50			// 敵の最大発生数

#define EF_DRAW			0x01		// 敵を描画するか
#define EF_CLIP			0x02		// 敵が画面外に出たとき消去するか
#define EF_DAMAGE		0x04		// 敵にダメージを与えられるか
#define EF_HITSB		0x08		// 敵と自機は接触するか
#define EF_RLCHG		0x10		// ＥＣＬ左右反転を有効にするか
#define EF_BOMB			0x20		// 敵が爆発中である
#define EF_DELETE		0x40		// 敵をこのターン中に消去する

#define ENEMY_BOMB_SPD	4


////アニメーション定数////
// (Yes, this is game logic. [ECL_ANM] directly derives hitbox sizes from the
// sprite sizes defined in [Anime].)
#define ANIME_MAX		50			// アニメーションの種類
#define ANIMEPTN_MAX	16			// アニメーションパターンの最大値
#define ANM_NORM		0x00		// 普通のアニメーション
#define ANM_DEG			0x01		// 角度でアニメーションする
#define ANM_STOP		0x02		// 最終パターンで静止する


//// 割り込みベクタ構造体 ////
struct INT_VECTOR {
	uint32_t	vect;	// 割り込みベクタ(0 なら無効)
	int		value;		// 比較値
};

//// 敵データ構造体 ////
struct ENEMY_DATA {
	WORLD_COORD	x, y;	// 表示座標
	int			vx,vy;		// 速度の(x,y)成分 x64系

	int			v;			// 速度成分 x64系

	uint32_t	hp;	// 残り体力(大きすぎるか？)
	uint32_t	item;	// アイテムその他に使用か？
	uint32_t	cmd;	// ECLコマンドの絶対アドレス(DOS版との大きな変更点)
	uint32_t	count;	// 多目的フレームカウンタ
	uint32_t	call_addr;	// RET 実行後にジャンプするアドレス

	uint32_t	score;	// 得点(時間による得点変動対応か？)
	uint32_t	evscore;	// かすり得点用

	uint32_t	IntTimer;	// 割り込みようタイマー

	uint32_t	GR[ECLREG_MAX];	// 変数用レジスタ
	INT_VECTOR	Vect[ECLVECT_MAX];	// 割り込みベクタ

	uint16_t	g_width;	// グラフィックの幅  /2*64(当たり判定にも使用)
	uint16_t	g_height;	// グラフィックの高さ/2*64(上に同じ)
	uint16_t	rep_c;	// REP 命令用カウンタ
	uint16_t	cmd_c;	// 実行中コマンドの繰り返し回数
	uint16_t	anm_c;	// アニメーションカウンタ

	uint8_t	d;	// 進行角 256
	char		vd;			// 角速度 128
	uint8_t	amp;	// 振幅   256
	uint8_t	anm_ptn;	// 使用しているアニメーションパターン
	uint8_t	anm_ptnEx;	// ダメージ中のアニメーションパターン
	char		anm_sp;		// アニメーションスピード
	uint8_t	IsDamaged;	// ダメージを受けたか

	uint8_t	flag;	// 敵状態フラグ(状況によってサイズ変更のこと)
	uint8_t	tama_c;	// 弾発射用カウンタ
	uint8_t	t_rep;	// 弾発射間隔

	uint8_t	LLaserRef;	// 太レーザーの参照カウント

	TAMA_CMD	t_cmd;		// 弾発射用コマンド
	LASER_CMD	l_cmd;		// レーザー発射用コマンド
};

using ENEMY_DATA_CSPAN = std::span<const ENEMY_DATA, ENEMY_MAX>;
using ENEMY_DATA_IND = IND_TYPE<ENEMY_MAX>;

struct ANIME_DATA {
	uint8_t	mode;	// アニメーションモード
	uint8_t	n;	// アニメーションパターン数
	PIXEL_SIZE	size;	// 絵の幅, 絵の高さ
	PIXEL_LTRB	ptn[ANIMEPTN_MAX];	// アニメーションの存在する矩形領域
};

class C_ENEMY {
private:
	HOOKS& Hooks;
	C_BOSS& Boss;
	C_EFFECT3D& Effect3D;
	C_HLASER& HLaser;
	C_ITEM& Item;
	C_LASER& Laser;
	C_LLASER& LLaser;
	C_PLAYRANK const& PlayRank;
	C_RNG& RNG;
	C_TAMA& Tama;
	C_VIV& Viv;

	//// 敵変数 ////
	ENEMY_DATA_IND EnemyNow;

	// 特殊角度の現在値
	uint8_t EnemyEXDEG;

	// 特殊角度の増分
	uint8_t EnemyEXDEG_D;

	BUFFER_OWNED ECL_Head;
	std::array<ENEMY_DATA, ENEMY_MAX> Enemy;
	std::array<ENEMY_DATA_IND, ENEMY_MAX> EnemyInd;
	ANIME_DATA Anime[ANIME_MAX];

	bool DamageApply(ENEMY_DATA& e, int damage);

public:
	C_ENEMY(
		HOOKS& Hooks,
		C_BOSS& Boss,
		C_EFFECT3D& Effect3D,
		C_HLASER& HLaser,
		C_ITEM& Item,
		C_LASER& Laser,
		C_LLASER& LLaser,
		C_PLAYRANK const& PlayRank,
		C_RNG& RNG,
		C_TAMA& Tama,
		C_VIV& Viv
	) noexcept :
		Hooks(Hooks),
		Boss(Boss),
		Effect3D(Effect3D),
		HLaser(HLaser),
		Item(Item),
		Laser(Laser),
		LLaser(LLaser),
		PlayRank(PlayRank),
		RNG(RNG),
		Tama(Tama),
		Viv(Viv) {
	}

	//// 敵制御関数 ////
	ENEMY_DATA *FindFree(void);

	// Returns `true` if [ecl] is a valid pointer.
	bool Set(BUFFER_OWNED&& ecl, uint8_t stage);

	// 敵データを初期化する(x,y は x64 で指定のこと) //
	void InitEnemyDataX64(ENEMY_DATA *e, int x, int y, uint32_t id);

	// 敵データを初期化する(x,y は非x64(ランダム可能) で指定のこと) //
	void InitEnemyDataSTD(ENEMY_DATA *e, short x, short y, uint32_t id);

	// 敵を動かす
	void Move(void);

	// 敵の順序設定用配列の初期化をする
	void IndSet(void);

	// 雑魚を消滅させる
	void Clear(void);

	// 敵にダメージを与える
	bool Damage(int x, int y, int damage);

	// ｙ上方向無限Ver.敵にダメージを与える
	bool Damage2(int x, int y, int damage);

	// ナナメレーザーの当たり判定
	void Damage3(int x, int y, uint8_t d);

	// すべての敵にダメージを与える
	void Damage4(int damage);

	void AnimeMove(ENEMY_DATA *e) const;
	void Delete(ENEMY_DATA& e);
	void Explode(ENEMY_DATA& e);


	// 敵をＥＣＬに従って動かす
	void ECL_Parse(ENEMY_DATA *e);

	// 強制的に ECL ブロック間を移動する //
	void ECL_LongJump(ENEMY_DATA *e, uint32_t EclID) const;

	decltype(EnemyNow) NumAlive(void) const {
		return EnemyNow;
	}

	const ENEMY_DATA_CSPAN Data(void) const {
		return Enemy;
	}

	const std::span<const ENEMY_DATA_IND> Inds(void) const {
		return std::span(EnemyInd).first(EnemyNow);
	}

	const ANIME_DATA& AnimeSheet(unsigned int id) const noexcept {
		return Anime[id];
	}
};


// Vivit ナナメレーザーの当たり判定 //
bool LaserHITCHK(const ENEMY_DATA *e, int ox, int oy, uint8_t d);

#endif
