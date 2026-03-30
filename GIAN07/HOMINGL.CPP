/*                                                                           */
/*   HomingL.cpp   長いレーザーの処理                                        */
/*                                                                           */
/*                                                                           */

#include "HOMINGL.H"
#include "GIAN.H"
#include "MAID.H"
#include "hatoyama/engine/snd.h"
#include "hatoyama/logic/ut_math.h"


uint16_t	HLaserNow;	// ホーミングレーザーの本数
HLaserData		HLaserBuf[HLASER_MAX];	// ホーミングレーザー格納バッファ

HLaserData		ActiveHL;		// 確保済みホーミングレーザー
HLaserData		FreeHL;			// 解放済みホーミングレーザー



// ホーミングレーザーの初期化を行う //
void HLaserInit(void)
{
	int		i;

	HLaserNow = 0;

	ActiveHL.Next = nullptr;
	FreeHL.Next   = HLaserBuf;

	for(i=0; i<HLASER_MAX-2; i++){
		HLaserBuf[i].Next = HLaserBuf + (i+1);
	}

	HLaserBuf[HLASER_MAX - 1].Next = nullptr;
}


// ホーミングレーザーをセットする //
void HLaserSet(const HLaserInfo *hinfo)
{
	int			i,j;
	uint8_t	deg;
	HLaserData	*p;

	// 1-n としているのは、角度設定のためね... //
	for(i=1; i<=(hinfo->n); i++){
		p = FreeHL.Next;
		if(p == nullptr) {
			return; // データを確保できない
		}

		// ポインタ結合を行う //
		FreeHL.Next   = FreeHL.Next->Next;
		p->Next       = ActiveHL.Next;
		ActiveHL.Next = p;
		HLaserNow++;

		p->v       = 64*4;	// 加速度セット
		p->a       = 10;		// 速度セット
		p->Count   = 0;		// フレームカウンタ
		p->Current = 0;		// 先頭
		p->Left    = 1;		// 残りホーミング回数

		p->c       = hinfo->c;		// 色
		p->Type    = hinfo->type;	// 種類
		p->State   = HLS_NORM;		// 状態

		if(hinfo->n&1) deg = hinfo->d + (i>>1)*(hinfo->dw)*(1-((i&1)<<1));
		else           deg = hinfo->d - ((hinfo->dw)>>1)+(i>>1)*(hinfo->dw)*(1-((i&1)<<1));

		// しっぽ情報を初期化する //
		for(j=0; j<HLASER_LEN*HLASER_SECTION; j++){
			p->p[j].x = hinfo->x;
			p->p[j].y = hinfo->y;
			p->p[j].d = deg;
		}
	}
}


// ホーミングレーザーを動作させる //
void HLaserMove(void)
{
	HLaserData	*hl;
	HLaserData	*temp;
	int			x,y;
	int			i, j;
	int			deg,deg2;

	// 次のフレームに移行する //
	for(hl = ActiveHL.Next; hl != nullptr; hl = hl->Next) {
		// 前回の先頭を一時保存する //
		x   = hl->p[hl->Current].x;
		y   = hl->p[hl->Current].y;
		deg = hl->p[hl->Current].d;

		// 共通の更新を行う //
		hl->Count++;
		hl->Current = HLASER_GETNEXT(hl->Current);

		// 種類別の移動処理 //
		switch(hl->Type){
			case(HL_TYPE1):
				deg2 = -deg + atan8(Viv.x-x, Viv.y-y);
				if(deg2<-128)     deg2 += 256;
				else if(deg2>128) deg2 -= 256;

				if(abs(deg2)<8){
					hl->Type = HL_NONE;
					Snd_SEPlay(17, hl->p[hl->Current].x);
				}
				else{
					if(hl->v > 2*64)
						hl->v -= hl->a;
					i = 1+(hl->Count)/32;
					i = (deg2*i)/32;
					if(i) deg = deg+i;
					else  deg = deg+deg2;
				}

				if(hl->Count > 120) hl->Type = HL_NONE;

				hl->p[hl->Current].d = deg;
				hl->p[hl->Current].x = x + cosl(deg,hl->v);
				hl->p[hl->Current].y = y + sinl(deg,hl->v);
			break;

			case(HL_NONE):
				hl->v += hl->a*2;
				hl->p[hl->Current].d = deg;
				hl->p[hl->Current].x = x + cosl(deg,hl->v);
				hl->p[hl->Current].y = y + sinl(deg,hl->v);
			break;

			default:
			break;
		}

		// 今回のしっぽを一時保存する //
		i = HLASER_GETNEXT(hl->Current);
		x = hl->p[i].x;
		y = hl->p[i].y;

		// 範囲外チェック //
		if(x<GX_MIN-4*64||x>GX_MAX+4*64||y<GY_MIN-4*64||y>GY_MAX+4*64){
			hl->State = HLS_DEAD;
			continue;
		}

		if(Viv.muteki) continue;

		auto ev_flag = false;
		for(j=0; j<HLASER_LEN*HLASER_SECTION; j++){
			x = hl->p[j].x;
			y = hl->p[j].y;

			// かすり判定 //
			if(HITCHK(x,Viv.x,HOMINGL_WIDTH+15*64) && HITCHK(y,Viv.y,HOMINGL_WIDTH+15*64)){
				ev_flag = true;
			}

			// 当たり判定 //
			if(HITCHK(x,Viv.x,HOMINGL_WIDTH*2/3) && HITCHK(y,Viv.y,HOMINGL_WIDTH*2/3)){
			//	hl->State = HLS_DEAD;	// こいつは消去
				MaidDead();				// 殺っておしまい
			}
		}
		if(ev_flag) evade_add(1);
	}

	// 不要なデータを削除する //
	for(hl = &ActiveHL; (hl->Next) != nullptr;) {
		if(hl->Next->State == HLS_DEAD){
			temp           = hl->Next->Next;
			hl->Next->Next = FreeHL.Next;
			FreeHL.Next    = hl->Next;
			hl->Next       = temp;

			HLaserNow--;
		}
		// そうでなければポインタを進める //
		else{
			hl = hl->Next;
		}
	}
}

// ホーミングレーザーに消去エフェクトをセット //
void HLaserClear(void)
{
//	HLaserData	*hl;

	HLaserInit();
	/*
	for(hl = ActiveHL.Next; hl != nullptr; hl = hl->Next) {
		hl->State = HLS_CLEAR;
	}
	*/
}
