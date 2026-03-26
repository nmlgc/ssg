/*
 *   Unused SCL processing code
 *
 */

// p:SCL_ENEMY以降の敵配置データ //
static void enemy_set(void)
{
	//	BIT_DEVICE	*in;				// やばやば...

	while (bFlag) {
		cmd = SCL_Now;
		switch (cmd[0]) {
		case(SCL_TIME):
			/*
							if((GetAsyncKeyState(VK_CONTROL)&0x8000)&&SclInfo.MsgFlag)
								GameCount+=(temp-GameCount)/3;
							else if((Key_Data&KEY_RETURN) && SclInfo.ReturnFlag==FALSE){
								GameCount  = temp;
								SclInfo.ReturnFlag = TRUE;
							}
							if(!(Key_Data&KEY_RETURN) && SclInfo.ReturnFlag) SclInfo.ReturnFlag = FALSE;
			*/
			break;

		case(SCL_MSG):		// メッセージを出力する
			//MWinCmd(MWCMD_SMALLFONT);
			break;

		case(SCL_END):		// カウントも変更させずにリターンする
			/*
			MWinOpen(&MWinRect);
			MWinCmd(MWCMD_NEWPAGE);
			MWinCmd(MWCMD_LARGEFONT);
			MWinMsg("ＳＣＬ完了ですの");
			SCL_DEBUG("--- SCL_END ---");
			*/
			return;

		case(SCL_EFC):
			switch (cmd[1]) {
			case(SEFC_WARN):
				//effect_set(0,0,EFC_WARNBOSS,GameStage);
				//StringEffect3(GameStage);
				break;
			}
			break;
		}
	}
}

static void _PutEnemy(BYTE *p)
{
/*
 * [メモ]
 *  p[0-1]:EnemyX  p[2-3]:EnemyY  p[4]:EnemyID
 *  ECL_Head[0-3]:Num  ECL_Head[n*4-(n*4+3)] (n>1):StartAddr(ABS)
 */
	short			x,y;

	x   = (*(short *)(&p[0]));	//((int)(*(short *)(&p[0])))*64;
	y   = (*(short *)(&p[2]));	//((int)(*(short *)(&p[2])))*64;

/*
	e->x   = (*(short *)(&p[0]));	//((int)(*(short *)(&p[0])))*64;
	e->y   = (*(short *)(&p[2]));	//((int)(*(short *)(&p[2])))*64;

	e->x = (e->x==X_RNDV) ? GX_RND : (e->x<<6);
	e->y = (e->y==Y_RNDV) ? GY_RND : (e->y<<6);
	e->cmd = (*(DWORD *)(&ECL_Head[n]));

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

	e->tama_c   = rnd();//&0xff;
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

/*
static void ExDraw(void)
{
	int			x,y;
	int			dx,dy;
	int			infx,infy;
	int			i,j;

	int			ox,oy;

	static BYTE		deg = 0;
	static int		len = 0;
	static BYTE		count = 0;
	static char		flag = 1;

	ox = cosl(deg,len*flag)+320;
	oy = sinl(deg,len*flag)+240;

	count++;

	if(count==0)	flag *= -1;

	if(count>60)	len = (len+4)%64;
	else			deg+=4;

	GrpLock();
	GrpSetColor(0,3,0);

	dx = cosl(deg+64,64);		infx = cosl(deg,800);
	dy = sinl(deg+64,64);		infy = sinl(deg,800);
	for(i=-10;i<=10;i++){
		x = ox + dx*i;
		y = oy + dy*i;
		GrpLine(x-infx,y-infy,x+infx,y+infy);
	}

	dx = cosl(deg,64);		infx = cosl(deg+64,800);
	dy = sinl(deg,64);		infy = sinl(deg+64,800);
	for(i=-10;i<=10;i++){
		x = ox + dx*i;
		y = oy + dy*i;
		GrpLine(x-infx,y-infy,x+infx,y+infy);
	}

	GrpUnlock();
}
*/

// 背景を描画する //
FVOID ScrollDraw(void)
{
	// 例外処理：ワイドショット用ボム発動中 //
	//if(Viv.bomb_time && Viv.weapon==0){
	//	return;
	//}

	// 振動エフェクト用 //
	//dx = sinl(ScrollInfo.IsQuake*4,2);
	//if(ScrollInfo.IsQuake) dx = sinl(ScrollInfo.IsQuake*8+i*6,(256-ScrollInfo.IsQuake)>>2);	//4
/*
	if(ScrollInfo.ExCmd==ScrollCmdStg2Boss){
		ExDraw();
	}
*/
}

// ＳＣＬ用コマンド実行関数(引数:(1)スクロールコマンド) //
FVOID ScrollCommand(BYTE cmd)
{
	switch (cmd) {
	case(SCMD_STG3BOSS):	// ３面ボス
		//InitStg3Cloud();
		break;
	}
}

// ゲイツ雲 //
static FVOID ScrollCmdStg3Boss(void)
{
	//	MoveStg3Cloud();
}

// ゲイツ雲描画 //
static FVOID Stg3BossMapDraw(void)
{
	//	DrawStg3Cloud();
}



// マップデータをロードする(BMP含む) //
FBOOL ScrollInit(void)
{
/*
	// 読み込みの準備 //
	in = FilStartR("GIAN_MAP.DAT",BD_FILE_READ);
	if(in==NULL) return FALSE;

	// もしすでにロードが行われていた場合、メモリを解放する //
	if(ScrollInfo.DataHead!=NULL){
		LocalFree(ScrollInfo.DataHead);
		ScrollInfo.DataHead = NULL;
	}
	// そうでなければ、スクロールに関する初期化を行う //
	else if(!InitScrollInfo()) return FALSE;

	// 解凍を行う(後で、ステージを考慮したものに変更すること) //
	ScrollInfo.DataHead = MemExpand(in,0);
	if(ScrollInfo.DataHead==NULL) return FALSE;
	FilEnd(in);
*/
}
