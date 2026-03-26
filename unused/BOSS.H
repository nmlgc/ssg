/*
 *   Unused boss code
 *
 */

///// [構造体] /////

// 特殊当たり判定 //
typedef struct tagEXHITCHK{
	BYTE	flags[60][60];
} EXHITCHK;

// ボスデータ //
typedef struct tagBOSS_DATA{
	EXHITCHK		*Hit;					// 特殊当たり判定(NULL なら使用しない)
} BOSS_DATA;
