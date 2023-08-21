/*                                                                           */
/*   EFFECT.h   エフェクト管理                                               */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_EFFECT_H
#define PBGWIN_EFFECT_H		"EFFECT : Version 0.20 : Update 2000/02/23"
//#pragma message(PBGWIN_EFFECT_H)

#include <stdint.h>
#include <string_view>



///// [更新履歴] /////
 // 2000/04/28 : 円エフェクトを作成
 // 2000/04/15 : 円形フェード系関数の追加
 // 2000/02/23 : 開発開始(->Ver0.20)



///// [ 定数 ] /////
#define SEFFECT_MAX		1000
#define LOCKON_MAX		2			// 最大ロック数
#define CIRCLE_EFC_MAX	10			// 円エフェクト最大数

#define CEFC_NONE		0x00		// CircleEffect未使用
#define CEFC_STAR		0x01		// お星様系エフェクト
#define CEFC_CIRCLE1	0x02		// 集まる円エフェクト
#define CEFC_CIRCLE2	0x03		// 離れる円エフェクト

#define SEFC_NONE		0x00		// 未使用
#define SEFC_STR1		0x01		// 文字列エフェクト１
#define SEFC_STR1_2		0x02		// 文字列一時停止
#define SEFC_STR1_3		0x03		// 文字列爆発

#define SEFC_MTITLE1	0x04		// 曲名表示エフェクト(出動)
#define SEFC_MTITLE2	0x05		// 曲名表示エフェクト(停止)
#define SEFC_MTITLE3	0x06		// 曲名表示エフェクト(退却)

#define SEFC_GAMEOVER	0x07		// ワーニングの表示とか
#define SEFC_GAMEOVER2	0x08		// ワーニングの表示とか
#define SEFC_GAMEOVER3	0x09		// ワーニングの表示とか

#define SEFC_STR2		0x10		// 得点アイテム用？エフェクト

#define LOCKON_NONE		0x00		// ロックしていない
#define LOCKON_01		0x01		// ロックオン開始
#define LOCKON_02		0x02		// ロックオン停止
#define LOCKON_03		0x03		// ロックオン解放？

#define SCNEFC_NONE			0x00	// エフェクト無し
#define SCNEFC_CFADEIN		0x01	// 円形フェードイン
#define SCNEFC_CFADEOUT		0x02	// 円形フェードアウト
#define SCNEFC_WHITEIN		0x03	// ホワイトイン
#define SCNEFC_WHITEOUT		0x04	// ホワイトアウト



///// [構造体] /////
typedef struct _tagCIRCLE_EFC_DATA{
	int		x,y;		// 中心座標
	int		r,rmax;		// 半径／最大半径
	uint32_t	count;	// カウンタ
	uint8_t	type;	// 円エフェクトの種類
//	uint8_t	Level;	// 円エフェクトの段階
	uint8_t	d;	// 円エフェクトの角度(謎)
} CIRCLE_EFC_DATA;

typedef struct _tagSEFFECT_DATA{
	int		x,y;
	int		vx,vy;

	uint32_t	time;
	uint32_t	point;

	uint8_t	cmd;
	char	c;
} SEFFECT_DATA;

typedef struct tagLOCKON_INFO{
	int		*x,*y;						// ロック座標へのポインタ
	int		width,height;				// 幅＆高さ
	int		vx,vy;						// 速度成分
	uint32_t	count;	// カウンタ
	uint8_t	state;	// 状態
} LOCKON_INFO;

typedef struct tagSCREENEFC_INFO{
	uint8_t	cmd;	// 発動中エフェクト
	uint32_t	count;	// エフェクトに対するカウンタ
} SCREENEFC_INFO;



///// [ 関数 ] /////
void MTitleInit(void);	// Registers the music title rectangle
void SEffectInit(void);	// エフェクトの初期化を行う
void StringEffect(int x, int y, const char *s);	// 文字列系エフェクト(上に表示する奴)
void StringEffect2(int x, int y, uint32_t point);	// 得点表示エフェクト
void StringEffect3(void);	// ゲームオーバーの表示
void SetMusicTitle(int y, std::string_view s);	// 曲名の表示

void SEffectMove(void);	// エフェクトを動かす(仕様変更の可能性があります)
void SEffectDraw(void);	// エフェクトを描画する(仕様変更の可能性があります)

void CEffectInit(void);	// 円エフェクトの初期化
void CEffectMove(void);	// 円エフェクトを動かす
void CEffectDraw(void);	// 円エフェクトを描画する
void CEffectSet(int x, int y, uint8_t type);	// 円エフェクトをセットする

void ScreenEffectInit(void);	// 画面全体に対するエフェクトの初期化
void ScreenEffectSet(uint8_t cmd);	// 画面全体に対するエフェクトをセットする
void ScreenEffectMove(void);	// 画面全体に対するエフェクトを動かす
void ScreenEffectDraw(void);	// 画面全体に対するエフェクトを描画する

void WarningEffectInit(void);	// ワーニングの初期化
void WarningEffectSet(void);	// ワーニング発動！！
void WarningEffectMove(void);	// ワーニングの動作
void WarningEffectDraw(void);	// ワーニングの描画

void ObjectLockOnInit(void);	// ロックオン配列を初期化
void ObjectLockOn(int *x, int *y, int wx64, int hx64);	// 何かをロックオンする
void ObjectLockMove(void);	// ロックオンアニメーション動作
void ObjectLockDraw(void);	// ロックオン枠描画

void GrpDrawSpect(int x, int y);	// スペアナ描画
void GrpDrawNote(void);	// 押されているところを表示



///// [ 変数 ] /////
extern SEFFECT_DATA		SEffect[SEFFECT_MAX];
extern CIRCLE_EFC_DATA	CEffect[CIRCLE_EFC_MAX];
extern LOCKON_INFO		LockInfo[LOCKON_MAX];
extern SCREENEFC_INFO	ScreenInfo;


#endif
