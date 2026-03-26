/*
 *   Unused player code
 *
 */

///// [ 定数 ] /////

// サボテン定数 //
#define VIVMUTEKI_VAL	(180)			// びびっとの無敵時間

//#define EVADE_X_START	150			// かすりカウンタの初期表示座標
//#define EVADE_X_SPD		5			// かすりカウンタの移動速度
//#define EVADE_TIME		60			// かすり許容フレーム数

#define SSP_WIDE		(64*9)		//
#define SSP_HOMING		(64*9)		//
#define SSP_LASER		(64*13)		//



///// [ 関数 ] /////
//extern FVOID evade_addEx(int x,int y,BYTE n);		// かすりゲージを上昇させる



///// [ 変数 ] /////
extern DWORD		EvadeRate[256];			// かすり得点レート
