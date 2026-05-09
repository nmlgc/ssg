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

	// Always was BossSTDMove() in the original game.
	FVOIDFN(*ExMove)(tagBOSS_DATA *);		// 特殊移動用関数
} BOSS_DATA;
