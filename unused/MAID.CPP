/*
 *   Unused player code
 *
 */

#include "Maid.h"

DWORD			EvadeRate[256];			// かすり得点レート

// 初期化 //
extern void MaidSet(void)
{
	int i;
	int sct[4] = {5,10,15,30};

	for(i=0;i<256;i++)
		EvadeRate[i] = i*sct[ConfigDat.GameLevel.v];
}
