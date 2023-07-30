/*
 *   Usage code for the unused double-spiral moiré animation
 *
 */

#include "FONTUTY.H"

// フォント情報格納用ね //
ExtraFontInfo	*pFontInfo[7];
ExtraFontInfo	*pMember[7];

bool EndingInit(void)
{
	int		i;

	for(i=0; i<7; i++){
		pFontInfo[i] = CreateExtraFont(GrEndingCredits, &src[i]);
		if(pFontInfo[i] == NULL) return FALSE;
	}

	for(i=0; i<7; i++){
		pMember[i] = CreateExtraFont(GrEndingCredits, &src2[i]);
		if(pMember[i] == NULL) return FALSE;
	}
}

// スタッフの描画 //
void DrawStfInfo()
{
	int		i;

	if(!EStfTask.bWantDisp) return;

	DrawExtraFont(pFontInfo[EStfTask.TitleID],
		EStfTask.ox, EStfTask.oy, 0);//255-EStfTask.alpha);

	for(i=0; i<EStfTask.NumStf; i++){
		DrawExtraFont(pMember[EStfTask.StfID[i]],
			EStfTask.ox, EStfTask.oy+i*30+50, 0);//255-EStfTask.alpha);
	}
}

// おしまい処理 //
void EndingCleanup()
{
	int		i;

	for(i=0; i<7; i++){
		DeleteExtraFont(pFontInfo[i]);
	}

	for(i=0; i<7; i++){
		DeleteExtraFont(pMember[i]);
	}
}
