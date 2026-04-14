/*************************************************************************************************/
/*   TAMA.H   たまに関する定義とかいろいろ                                                       */
/*                                                                                               */
/*************************************************************************************************/

#ifndef PBGWIN_TAMA_H
#define PBGWIN_TAMA_H		"TAMA : Ver 0.62 : Update 2000/02/17"
//#pragma message(PBGWIN_TAMA_H)

#include "GIAN07/entity.h"
#include "hatoyama/logic/coords.h"


///// [更新履歴] /////


/* -> ここから、ちと古いよ(1999...)
 * 変更しなけりゃいけないこと、とか
 * >ox,oy を廃止すること(TAMA.C も変更しなきゃならんし、面倒だな...)
 *
 * > 5/17 (2:43)  : ox,oy を廃止した。 弾の種類によっては自動的にクリッピングをするようにした
 *                : 上の奴との関連により .flag の機能を拡張しました(下の定数参照)
 *
 * > 6/13 (8:05)  : どっと単位のクリッピング
 */



////弾定数////
#define TAMA_MAX	(801*3)		// 弾の最大発生数
#define TAMA_EVADE	1			// 弾のかすり値

#define TAMA1_POINT		10000				// 弾の得点
#define TAMA2_POINT		15000				// 弾の得点

#define TAMA_HITX		(2*64)			// 弾の当たり判定
#define TAMA_HITY		(4*64)			// 弾の当たり判定
#define TAMA_EVX_SMALL	(( 8+ 8)*64)	// たま（小）のかすり判定(x)
#define TAMA_EVY_SMALL	((16+ 8)*64)	// たま（小）のかすり判定(y)
#define TAMA_EVX_LARGE	(( 8+16)*64)	// たま（大）のかすり判定(x)
#define TAMA_EVY_LARGE	((16+16)*64)	// たま（大）のかすり判定(y)

#define TAMA_SMALL	0x00	// 弾が小型弾である場合の上位４ビット
#define TAMA_LARGE	0x10	// 弾が大型弾である場合の上位４ビット
#define TAMA_ANGLE	0x20	// 弾が方向指定系である場合の上位４ビット
#define TAMA_EXTRA	0x30	// 弾がエキストラ用である場合の上位４ビット
#define TAMA_EXTRA2	0x40	// 弾が「おふだ」である場合の上位４ビット
#define TAMA_REN	0x04	// 弾の連射属性
#define TAMA_ZSET	0x08	// 弾のサボテン(自機)セット属性
#define TAMASP_RND0	0x00	// 速度ランダム無し
#define TAMASP_RND1	0x40	// 速度ランダム？？
#define TAMASP_RND2	0x80	// 速度ランダム？？
#define TAMASP_RND3	0xc0	// 速度ランダム？？


////弾の種類定数(上位４ビットは現在、使用目的がない)////
#define T_NORM		0x00	// 通常弾			:(vx,vy)で移動します
#define T_NORM_A	0x01	// 加速弾			:rep 加速回数?
#define T_HOMING	0x02	// ｎ回ホーミング	:rep ホーミング回数 / a 加速度
#define T_HOMING_M	0x03	// ｎ％ホーミング	:a 加速度 / vd ホーミング率
#define T_ROLL		0x04	// 回転弾			:rep 回転時間 / vd 角速度
#define T_ROLL_A	0x05	// 回転弾(加速)		:上の奴 + a 加速度
#define T_ROLL_R	0x06	// 回転弾(反転)		:上の奴と同じ
#define T_GRAVITY	0x07	// 落下弾			:(vx,vy)＆vyに(加速度a)がかかる
#define T_CHANGE	0x08	// 角度変更弾		:rep フレームでvdに角度変更
#define T_SBHOMING	0x09	// サボテン用ホーミング
#define T_SBHBOMB	0x0a	// サボテン用ホーミングボム


////弾オプション定数(下位４ビットはオプションの成分指定用)////
#define TOP_NONE	0x00	// オプションなし
#define TOP_WAVE	0x10	// 波		: 振幅
#define TOP_ROLL	0x20	// 回転		: 回転半径
#define TOP_PURU	0x30	// ぷるぷる	: ぷるぷる度
#define TOP_REFX	0x40	// 反射Ｘ	: 反射回数
#define TOP_REFY	0x50	// 反射Ｙ	: 反射回数
#define TOP_REFXY	0x60	// 反射ＸＹ	: 反射回数
#define TOP_DIV		0x70	// 分裂		: 分裂時の弾コマンド
#define TOP_BOMB	0x80	// ボム???	: 爆発半径


////弾コマンド定数////
#define TC_WAY		0x00	// 扇状発射
#define TC_ALL		0x01	// 全方向発射
#define TC_RND		0x02	// 基本角セット有りランダム
#define TC_WAYS		0x04	// 扇状発射＆連射
#define TC_ALLS		0x05	// 全方向発射＆連射
#define TC_RNDS		0x06	// 基本角セット有りランダム＆連射
#define TC_WAYZ		0x08	// 扇状発射＆サボテンセット
#define TC_ALLZ		0x09	// 全方向発射＆サボテンセット
#define TC_RNDZ		0x0a	// 基本角サボテンセットランダム
#define TC_WAYSZ	0x0c	// 扇状発射＆連射＆サボテンセット
#define TC_ALLSZ	0x0d	// 全方向発射＆連射＆サボテンセット
#define TC_RNDSZ	0x0e	// 基本角サボテンセットランダム＆連射


////弾エフェクト定数(下位４ビットの使用方法は現在考案中!!)////
#define TE_NONE		0x00	// エフェクトなし
#define TE_ROLL1	0x10	// 回転ためエフェクト
#define TE_ROLL2	0x20	// 回転ためエフェクト
#define TE_WARN		0x30	// Warning表示
#define TE_ROCK		0x40	// ロックオン
#define TE_CIRCLE1	0x50	// わっかエフェクト(小->大)
#define TE_CIRCLE2	0x60	// わっかエフェクト(大->小)
#define TE_DELETE	0xf0	// 消去エフェクト


////弾フラグ定数////
#define TF_NONE		0x00	// フラグが立っていない状態
#define TF_CLIP		0x01	// 画面外に出ても消去しない
#define TF_EVADE	0x02	// 一回かすっている場合
#define TF_DELETE	0x80	// その弾を消去する



////弾コマンド構造体(安全性アップ)////
struct TAMA_CMD {
	int  x,y;		// 弾の発射位置

	uint8_t d;	// 発射角
	uint8_t dw;	// 発射幅
	uint8_t n;	// 弾数(ｎ方向に発射)
	uint8_t ns;	// 連射数(cmdのsビットがONのときだけ有効)
	uint8_t v;	// 速度(下位６ビット)＆ランダム要素(上位２ビット)
	uint8_t c;	// 弾の色＆形状
	char a;			// 加速度(速度とは単位が違うので注意)

	char vd;		// 角速度｜ホーミング率(BYTE にキャスト)

	uint8_t rep;	// 繰り返し回数(回転、ｎ回ホーミング等)
	uint8_t cmd;	// 弾コマンド＆エフェクト
	uint8_t type;	// 弾の種類
	uint8_t option;	// 弾の属性(バイブレーション,反射,炸裂,ボム)
};



////弾データ構造体////
struct TAMA_DATA {
	int x,y;		// 現在の<表示>座標
	int tx,ty;		// 振動系エフェクト使用時の演算用座標
	int vx,vy;		// 速度の(X,Y)成分

 	int  v;			// 速度
	int  v0;		// 初速度(回転系エフェクト等で使用)
	char a;			// 加速度

	uint8_t d;	// 進行角
	uint16_t d16;	// 進行角(固定小数点 x256) -> ｎ％ホーミングでのみ使用する
	int8_t vd;	// 角速度

	uint8_t c;	// 弾の色＆形状

	uint8_t rep;	// typeによる制御を行う回数
	uint8_t type;	// 弾の種類(通常,加速,ホーミング2,回転3,落下,変更)
	uint8_t option;	// 弾の属性(バイブレーション,反射,炸裂,ボム)
	uint8_t effect;	// 実行中のエフェクト(なし,ロック,サークル,消去)

	uint16_t count;	// フレームカウンタ
	uint8_t flag;	// 弾消去要請フラグ
};

using TAMA_DATA_CSPAN = std::span<const TAMA_DATA, TAMA_MAX>;



class C_TAMA {
public:
	// 標準・弾コマンド構造体
	TAMA_CMD TamaCmd;

private:
	////弾の各種変数たち////

	// 小型弾の弾数
	uint16_t Tama1Now;

	// 特殊弾の弾数
	uint16_t Tama2Now;

	// 小型弾の最大数
	uint16_t Tama1Max;

	// 特殊弾の最大数
	uint16_t Tama2Max;

	int TamaSpeed;

	// ホーミング対象のＸ座標
	WORLD_COORD HomingX;

	// ホーミング対象のＹ座標
	WORLD_COORD HomingY;

	// 真ならホーミング実行
	int HomingFlag;

	// 弾の格納用構造体
	std::array<TAMA_DATA, TAMA_MAX> Tama;

	// 小型弾の順番を維持するための配列
	std::array<uint16_t, TAMA_MAX> Tama1Ind;

	// 特殊弾の順番を維持するための配列
	std::array<uint16_t, TAMA_MAX> Tama2Ind;

	// 弾の進行方向をセットする
	uint8_t tama_dir(uint16_t i);

	// 弾の初速度をセットする
	int tama_speed(uint16_t i, int vret, const TAMA_CMD& TamaCmd);

	// 弾のフラグを初期化する
	uint8_t tama_flag(void);

	int LineCmdNewTamaSpeed(uint16_t i);
	int TamaSpeedEx(uint8_t d);

	void __Set(void);

public:
	/// Internal interface, shared between TAMA and MAIDTAMA
	/// ----------------------------------------------------

	// Initializes an individual (regular) bullet or player shot from `TamaCmd`.
	void SetSingle(
		TAMA_DATA& t,
		unsigned int i,
		int speed,
		decltype(TAMA_DATA::effect) effect
	);

	// 弾の " Type " による移動を行う
	void Tmove(TAMA_DATA *t);

	// 弾の "Option" による移動を行う
	void Omove(TAMA_DATA *t);

	// 弾の "Effect" による移動を行う
	void Emove(TAMA_DATA *t);
	/// ----------------------------------------------------

public:
	////弾関数////

	// 弾をセットする(難易度による変更は"有り")
	void Set(void);

	// 弾をセットする(難易度による変更は"無し")
	void SetEX(void);

	// 弾をセットする(ライン状に発射)
	void SetLine(void);

	// エキストラボス専用弾幕
	void SetExtra01(void);

	// 弾を動かす
	void Move(void);

	// 弾に消去エフェクトをセットする
	void Clear(void);

	// 弾の個数の割合をセットする(危険！)
	void IndSet(uint16_t tama1);

	// 弾を得点化する(Ret : 得点)
	uint32_t ToScore(void);

	// 弾の 1/n をアイテム化する
	void ToItem(uint8_t n);

	void ResetHoming(void);

	// ホーミング座標を更新する
	void UpdateHoming(const WORLD_POINT& p);

	TAMA_DATA_CSPAN Data(void) const {
		return Tama;
	}

	const std::span<const uint16_t> Inds1(void) const {
		return std::span(Tama1Ind).first(Tama1Now);
	}

	const std::span<const uint16_t> Inds2(void) const {
		return std::span(Tama2Ind).first(Tama2Now);
	}
};

extern C_TAMA Tama;


template <size_t N> void Indsort(
	std::array<uint16_t, N>& indices,
	uint16_t& count,
	const std::array<TAMA_DATA, N>& entities
) {
	Indsort(indices, count, entities, [](const TAMA_DATA& t) {
		return (t.flag & TF_DELETE);
	});
}


#endif
