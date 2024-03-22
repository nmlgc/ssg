/*
 *   Unused enemy code
 *
 */

#include "ENEMY.H"

// The original game defined this function for rotational 48×48 and 80×80 cels
// whose 16 sprites were stored on two lines instead of one, but the original
// game didn't use any.
extern void SetAnime48(int x,int y,int id)
{
	int			i;
	ANIME_DATA	*a = Anime+id;

	a->size = { 48, 48 };
	a->n     = 16;
	a->mode  = ANM_DEG;

	for(i=0;i<16;i++){
		a->ptn[i].top    = y+(i/8)*48;
		a->ptn[i].left   = x+(i%8)*48;
		a->ptn[i].bottom = a->ptn[i].top  + 48;
		a->ptn[i].right  = a->ptn[i].left + 48;
	}
}

extern void SetAnime80(int x,int y,int id)
{
	int			i;
	ANIME_DATA	*a = Anime+id;

	a->size = { 80, 80 };
	a->n     = 16;
	a->mode  = ANM_DEG;

	for(i=0;i<16;i++){
		a->ptn[i].top    = y+(i/8)*80;
		a->ptn[i].left   = x+(i%8)*80;
		a->ptn[i].bottom = a->ptn[i].top  + 80;
		a->ptn[i].right  = a->ptn[i].left + 80;
	}
}
