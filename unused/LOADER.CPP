/*
 *   Unused loading code
 *
 */

extern void LoadPaletteFromEnemy(void)
{
/*
	LPDIRECTDRAWPALETTE		lpdp;

	if(DxObj.Bpp != 8) return;
	if(!GrMap) return;
	if(GrEnemy->GetPalette(&lpdp) != DD_OK) return;
/*
	if(bIsBombPalette){
		lpdp->GetEntries(0, 0, 256, tempPalette);
	}
	else{
*/
/*
		lpdp->GetEntries(0,0,256,DxObj.pe);
		GrpSetPalette(DxObj.pe);
//	}

	lpdp->Release();
*/
}

/*
// ボム用パレット属性に変更する //
extern void EnterBombPalette(void)
{
	LPDIRECTDRAWPALETTE		lpdp;

	bIsBombPalette = TRUE;

	if(DxObj.Bpp != 8) return;
	if(!GrBomber) return;
	if(GrBomber->GetPalette(&lpdp) != DD_OK) return;

	// 現在のパレットを記憶する //
	GrpGetPalette(tempPalette);

	lpdp->GetEntries(0,0,256,DxObj.pe);
	GrpSetPalette(DxObj.pe);

	lpdp->Release();
}


// ボム用パレット属性を外す
extern void LeaveBombPalette(void)
{
	if(!bIsBombPalette) return;

	bIsBombPalette = FALSE;

	GrpSetPalette(tempPalette);
}
*/
