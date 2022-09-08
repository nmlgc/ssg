/*                                                                           */
/*   DX_ERROR.c   DirectX のエラー出力用関数                                 */
/*                                                                           */
/*                                                                           */

#include "DX_ERROR.H"
#include <stdio.h>
#include <string.h>
#include <time.h>
#pragma message(PBGWIN_DX_ERROR_H)


// グローバル変数 //
static char			ErrorOut[1000];


extern void DebugSetup(const char *filename)
{
	char buf[1000];
	FILE *fp;

	//if(ErrorFP) return;
	strcpy(ErrorOut,filename);

	fp = fopen(ErrorOut,"a");
	if(fp==NULL) return;
	fseek(fp,0,SEEK_END);
	_strdate(buf);
	fprintf(fp,"[%s]",buf);
	_strtime(buf);
	fprintf(fp,"[%s]\n",buf);
	fclose(fp);
}

extern void DebugCleanup(void)
{
	strcpy(ErrorOut,"");
	//if(ErrorFP!=NULL) fclose(ErrorFP);
	//ErrorFP=NULL;
}

extern void DebugOut(const char *s)
{
	FILE *fp;
	fp= fopen(ErrorOut,"a");
	if(fp==NULL) return;
	fseek(fp,0,SEEK_END);
	fprintf(fp,"Error : %s\n",s);
	fclose(fp);
}
