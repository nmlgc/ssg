/*
 *   Unused Direct3D graphics code
 *
 */

// Text boxes were originally supposed to be textured?
FVOID GrpTexBox(int x1,int y1,int x2,int y2)
{
	if(!bLocked) return;

	Set2DPoint(work+0,x1,y1);work[0].tu = 0;work[0].tv = 0;
	Set2DPoint(work+1,x2,y1);work[1].tu = 1;work[1].tv = 0;
	Set2DPoint(work+2,x1,y2);work[2].tu = 0;work[2].tv = 1;
	Set2DPoint(work+3,x2,y2);work[3].tu = 1;work[3].tv = 1;

	DxObj.D3Dev->DrawPrimitive(D3DPT_TRIANGLESTRIP,D3DVT_TLVERTEX,work,4,D3DDP_WAIT);
}

FVOID GrpTexBoxEx(POINT *p)
{
	int i;

	if(!bLocked) return;

	for(i=0;i<4;i++){
		Set2DPoint(work+i,p[i].x,p[i].y);
	}

	work[0].tu = 0; work[0].tv = 0;
	work[1].tu = 1; work[1].tv = 0;
	work[2].tu = 1; work[2].tv = 1;
	work[3].tu = 0; work[3].tv = 1;

	DxObj.D3Dev->DrawPrimitive(D3DPT_TRIANGLEFAN,D3DVT_TLVERTEX,work,4,D3DDP_WAIT);

	AlphaEnable(FALSE);
}
