/*                                                                           */
/*   EnemyExCtrl.h   敵用の特殊処理                                          */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_ENEMYEXCTRL_H
#define PBGWIN_ENEMYEXCTRL_H		"ENEMYEXCTRL : Version 0.01 : Update 2000/02/27"
//#pragma message(PBGWIN_ENEMYEXCTRL_H)

#include "EXDEF.H"
#include "ENEMY.H"
#include "BOSS.H"


///// [更新履歴] /////

// 2000/12/13 : ビット用の関数の動作が安定する(時間を使いすぎね...)
// 2000/12/09 : ビット用の関数を追加する
// 2000/04/06 : 開発を始める



///// [ 定数 ] /////
#define SNAKE_MAX				4			// 蛇型の敵の最大数
constexpr auto SNAKEYMOVE_POINTS_PER_ENEMY = 8;

#define BIT_MAX					6			// ビットの最大数
#define BITCMD_STDMOVE			0x00		// 通常の移動を行う
#define BITCMD_CHGSPD			0x01		// 回転速度を変更する
#define BITCMD_SELECTATK		0x02		// 攻撃コマンドを変更する
#define BITCMD_CHGRADIUS		0x03		// 半径を変更する
#define BITCMD_MOVTARGET		0x04		// 目標(びびっと)に向けてブーメラン移動
#define BITCMD_DISABLE			0xff		// ビットを使用していない

#define BLASERCMD_OPEN			0x00		// レーザーをオープンする
#define BLASERCMD_CLOSE			0x01		// ビットの放っているレーザーをクローズする
#define BLASERCMD_CLOSEL		0x02		// ライン状態に推移させる

#define BLASERCMD_TYPE_A		0x03		// 一方向・角度固定レーザーを放射
#define BLASERCMD_TYPE_B		0x04		// 両方向角度同期変化レーザーを放射
#define BLASERCMD_TYPE_C		0x05		// 角度同期ｎ芒星レーザー
#define BLASERCMD_DISABLE		0xff		// 何も発動していない




///// [構造体] /////

// 蛇型の敵を管理する構造体 //
template <size_t Len> struct SNAKYMOVE_DATA {
	// 頂点バッファ(ExDef.h)
	DegPoint	PointBuffer[Len * SNAKEYMOVE_POINTS_PER_ENEMY];

	ENEMY_DATA	*EnemyPtr[Len];	// 尾となるデータ配列
	BOSS_DATA		*Parent;							// 親(頭となるデータ)
	size_t	Head;	// 頭を格納している地点のポインタ
	bool	bIsUse;	// この構造体を使用しているか

	constexpr static size_t Length() {
		return Len;
	}
};


typedef struct tagBIT_PARAM {
	ENEMY_DATA	*pEnemy;	// 対象となる敵へのポインタ

	uint32_t	BitHP;	// ビットの耐久力
	uint8_t	BitID;	// 基準角から何番目(0～)に相当するビットか
	uint8_t	Angle;	// 現在の角度
	char		Force;		// 現在力の加えられている方向
} BIT_PARAM;

typedef struct tagBIT_DATA {
	BIT_PARAM		Bit[BIT_MAX];		// ビットデータへのポインタ
	BOSS_DATA		*Parent;			// 親データへのポインタ

	int				x, y;				// 回転の中心(基本的には、Parent->x, Parent->y)
	int				v;					// 加速度付き移動時の速度
	int				a;					// 加速度付き移動時の加速度

	uint8_t	d;	// 加速度付き移動時の進行角度
	uint8_t	NumBits;	// 初期ビット数

	//uint16_t	DeltaAngle;	// ビット間の差分角

	int				Length;				// ビットの回転半径
	int				FinalLength;		// 最終目標とする回転半径

	char			BitSpeed;			// ビットの基本回転速度
	uint8_t	State;	// このビット集合の状態
	uint8_t	LaserState;	// レーザーの状態
	//uint16_t	ForceCount;	// 反動の残り時間

	uint16_t	BaseAngle;	// ビットの回転基本角

	bool	bIsLaserEnable;	// レーザーが動作中かどうか
} BIT_DATA;



///// [ 関数 ] /////
void SnakyInit(void);	// 蛇型の敵配列の初期化
void SnakySet(BOSS_DATA *b, int len, uint32_t TailID);	// 蛇型の敵をセットする
void SnakyMove(void);	// 蛇型の敵の移動処理
void SnakyDelete(const BOSS_DATA *b);	// 蛇型の敵を殺す

void BitInit(void);	// ビット配列の初期化
void BitSet(BOSS_DATA *b, uint8_t NumBits, uint32_t BitID);	// ビットをセットする
void BitMove(void);	// ビットを動作させる
void BitDelete(void);	// ビットを消滅させる
void BitLineDraw(void);	// ビット間のラインを描画する
void BitSelectAttack(uint32_t BitID);	// 攻撃パターンをセットor変更
void BitLaserCommand(uint8_t Command);	// レーザー系命令を発行
void BitSendCommand(uint8_t Command, int Param);	// ビット命令を送信
int  BitGetNum(void);	// 現在のビット数を取得する



#endif
