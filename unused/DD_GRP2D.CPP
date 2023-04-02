/*
 *   Unused snippets from the original DD_GRP2D component
 *
 */

#include "DD_GRP2D.H"

// ガンマコントロール //
FVOID _2DGamma(BYTE r,BYTE g,BYTE b,BYTE a)
{
	PALETTEENTRY		pe[256];
	int					i;

	for(i=0;i<256;i++){
		pe[i].peRed   = min(255,DxObj.pe[i].peRed   * (r+1) * (a+1) / (256*128));
		pe[i].peGreen = min(255,DxObj.pe[i].peGreen * (g+1) * (a+1) / (256*128));
		pe[i].peBlue  = min(255,DxObj.pe[i].peBlue  * (b+1) * (a+1) / (256*128));
		pe[i].peFlags = DxObj.pe[i].peFlags;
	}

	DxObj.Pal->SetEntries(0,0,256,pe);
	DxObj.Back->SetPalette(DxObj.Pal);
}
