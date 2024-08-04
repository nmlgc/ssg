/*
 *   Unused general graphics code
 *
 */

// 直接番号指定で矩形内を塗りつぶす //
FVOID GrpClsEx(RECT *Target, DWORD col)
{
	DDBLTFX		ddbltfx;
	HRESULT		ddrval;

	ZeroMemory(&ddbltfx,sizeof(ddbltfx));
	ddbltfx.dwSize      = sizeof(ddbltfx);
	ddbltfx.dwFillColor = col;

	for(;;){
		ddrval = DxObj.Back->Blt(Target,NULL,NULL,DDBLT_COLORFILL,&ddbltfx);

		if(ddrval == DD_OK)
			break;

		if(ddrval == DDERR_SURFACELOST){
			if(!GrpRestore())
				return;
		}

		if(ddrval != DDERR_WASSTILLDRAWING)
			break;
	}
}
