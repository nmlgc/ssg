/*
 *   Unused boss code
 *
 */

///// [構造体] /////

// ボスデータ配列を初期化する(中断、ステージクリア時に使用) //
FVOID BossDataInit(void)
{
	int			i;

	for(i=0;i<BOSS_MAX;i++){
		// 特殊変数の初期化.... //
		Boss[i].Hit      = NULL;			// 特殊当たり判定
	}
}

// ボスを描画する
FVOID BossDraw(void)
{
/*
	RECT	Right[6] = {
		{240+40, 400, 240+80, 480}, {320, 400, 320+32, 480},
		{384+24, 400, 384+48, 480}, {432+8, 400, 432+16, 480},
		{464+8,  400, 464+16, 480}, {480+8, 400, 480+16,  480},
	};

	RECT	Left[6] = {
		{240, 400, 240+40, 480}, {320, 400, 320+32, 480},
		{384, 400, 384+48, 480}, {432, 400, 432+16, 480},
		{464, 400, 464+16,  480}, {480, 400, 480+16,  480},
	};

	WORD	size[6] = {
		40, 32, 24, 16, 8, 8
	};
*/
}

static int PutBoss(int x,int y,DWORD id)
{
	DWORD			addr;
	ENEMY_DATA		*e;

	//if(EnemyNow+1>=ENEMY_MAX) return;
	//e = Enemy+ (*(EnemyInd+EnemyNow));
	//EnemyNow++;
	//n      = 4 + (((BYTE)p[4])<<2);
	//e->x   = (*(short *)(&p[0]));	//((int)(*(short *)(&p[0])))*64;
	//e->y   = (*(short *)(&p[2]));	//((int)(*(short *)(&p[2])))*64;

	/*
	e->x = x;
	e->y = y;

	// ランダム配置に対応するぞ //
	//e->x = (e->x==X_RNDV) ? GX_RND : (e->x<<6);
	//e->y = (e->y==Y_RNDV) ? GY_RND : (e->y<<6);
	addr   = 4 + (id<<2);		// ちと、やばいね...
	e->cmd = (*(DWORD *)(&ECL_Head[addr]));

	e->call_addr = e->cmd;

	e->hp       = 0xffffffff;
	e->amp      = 0;
	e->anm_ptn  = 0;
	e->anm_sp   = 0;
	e->anm_c    = 0;
	e->count    = 0;
	e->evscore  = 0;
	e->d        = 64;
	e->flag     = EF_DAMAGE|EF_DRAW|EF_HITSB;

	e->tama_c   = rnd()&0xff;
	e->t_rep    = 0;			// 弾の発射間隔(０：自動発射しない)
	e->g_width  = 0;
	e->g_height = 0;

	e->item     = 0;

	e->rep_c    = 0;
	e->cmd_c    = 0;
	e->v        = 64;
	e->vd       = 0;
	e->vx       = cosl(e->d,e->v);
	e->vy       = sinl(e->d,e->v);

	e->LLaserRef = 0;

	e->t_cmd.c      = 0;
	e->t_cmd.cmd    = TC_WAY;
	e->t_cmd.d      = 64;
	e->t_cmd.n      = 1;
	e->t_cmd.option = TE_NONE;
	e->t_cmd.type   = T_NORM;
	e->t_cmd.v      = 3;
	e->t_cmd.x      = 0;
	e->t_cmd.y      = 0;

	e->t_cmd.dw     = 16;
	e->t_cmd.ns     = 1;
	e->t_cmd.rep    = 0;
	e->t_cmd.vd     = 0;

	// 変数用レジスタの初期化 //
	e->GR[0] = e->GR[1] = e->GR[2] = e->GR[3] = 0;
	e->GR[4] = e->GR[5] = e->GR[6] = e->GR[7] = 0;

	// 割り込みベクタの初期化 //
	InitECLInterrupt(e);
*/
}
