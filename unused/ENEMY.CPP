/*
 *   Unused enemy code
 *
 */

#include "ENEMY.H"

/*
_inline Debug(DWORD old,int id)
{
	RndBuf[id] += (random_ref-old);
}
*/

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

static void _EnemyDrawBomb(int x,int y,DWORD count)
{
	RECT		src;

/*
	switch(count/ENEMY_BOMB_SPD){
		case(0):case(1):
			BltSetRect(&src,520,104,8,8);
			x-=4;y-=4;
		break;

		case(2):case(3):
			BltSetRect(&src,528,104,16,16);
			x-=8;y-=8;
		break;

		case(4):case(5):
			BltSetRect(&src,544,104,24,24);
			x-=12;y-=12;
		break;

		case(6):
			BltSetRect(&src,568,104,32,32);
			x-=16;y-=16;
		break;

		case(7):
			BltSetRect(&src,600,104,48,48);
			x-=24;y-=24;
		break;
	}
*/
}
