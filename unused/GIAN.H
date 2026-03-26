/*
 *   Unused game management declarations
 *
 */

// 座標関連 //
#define RL_WIDX		(32-4)					// 反射レーザー用_Ｘ座標_補正値
#define RL_WIDY		16						// 反射レーザー用_Ｙ座標_補正値
#define RLX_MIN		(GX_MIN/64+RL_WIDX)		// 反射レーザーの反射Ｘ座標最小値
#define RLX_MAX		(GX_MAX/64-RL_WIDX)		// 反射レーザーの反射Ｘ座標最大値
#define RLY_MIN		(GY_MIN/64+RL_WIDY)		// 反射レーザーの反射Ｙ座標最小値
#define RLY_MAX		(GX_MAX/64-RL_WIDY)		// 反射レーザーの反射Ｙ座標最大値

#define NREG_SX		(X_MID-13*9)			// ネームレジスト用ウィンドウの開始Ｘ
#define NREG_SY		(Y_MID+100)				// ネームレジスト用ウィンドウの開始Ｙ
#define NREGI_X		(X_MID-8*7)				// ネームレジスト用ウィンドウ(名前表示部)の開始Ｘ
#define NREGI_Y		(Y_MID+60)				// ネームレジスト用ウィンドウ(名前表示部)の開始Ｙ

#define STG_RNDXY	0				// 配置する座標がランダムの時の値...(なんだかよう分からん)

// スコア //
#define SCORE_NAME		9			// スコア用の最大文字列長(NULL含む)


// フラグ //
#define GRPF_ALPHA_ENABLE		0x01		// 半透明が有効(無効時はメッシュ???)

#define INPF_JOYPAD_ENABLE		0x01		// パッドが有効



///// [マクロ] /////
#define WAVESP(v) ((short)((v) << 4)) // WAVE?用の速度セット(まずいな)


///// [構造体] /////

/*
// スコア管理用構造体 //
typedef struct tagSCORE_DATA{
	int64_t	score;
	uint8_t	weapon;
	char	name[SCORE_NAME];
} SCORE_DATA;

typedef struct tagHIGH_SCORE{
	SCORE_DATA		easy[8];
	SCORE_DATA		normal[8];
	SCORE_DATA		hard[8];
	SCORE_DATA		lunatic[8];
} HIGH_SCORE;
*/


///// [グローバル変数] /////
//extern HIGH_SCORE	*HighScore;
//extern char			ScoreTable[8][80];
