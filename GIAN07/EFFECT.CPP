/*                                                                           */
/*   EFFECT.cpp   エフェクト管理                                             */
/*                                                                           */
/*                                                                           */

#include "EFFECT.H"
#include "FONTUTY.H"
#include "GEOMETRY.H"
#include "GIAN.H"
#include "game/cast.h"
#include "game/snd.h"
#include "game/ut_math.h"


SEFFECT_DATA		SEffect[SEFFECT_MAX];
CIRCLE_EFC_DATA		CEffect[CIRCLE_EFC_MAX];
LOCKON_INFO			LockInfo[LOCKON_MAX];
SCREENEFC_INFO		ScreenInfo;
TEXTRENDER_RECT_ID	MTitleRect;


extern void CircleFadeOut(int x, int y, int r);



// 円エフェクトの初期化 //
void CEffectInit(void)
{
	for(auto& it : CEffect) {
		it.type = CEFC_NONE;
	}
}

// 円エフェクトを動かす //
void CEffectMove(void)
{
	for(auto& it : CEffect) {
		auto *ce = &it;
		ce->count++;

		switch(ce->type){
			case(CEFC_STAR):
				ce->r-=3;
				ce->d+=2;
				if(ce->r <= 0){
					ce->type = CEFC_NONE;
				}
			break;

			case(CEFC_CIRCLE1):		// 集
				ce->r-=(10+5);
				if(ce->r <= 0){
					ce->type = CEFC_NONE;
				}
			break;

			case(CEFC_CIRCLE2):		// 離
				ce->r+=(8+5);
				if(ce->r >= ce->rmax){
					ce->type = CEFC_NONE;
				}
			break;
		}
	}
}

// 円エフェクトを描画する //
void CEffectDraw(void)
{
	int j, r;
	int					x1, x2, y1, y2;
	static BYTE dtable[4] = {0, 1, 3, 7};

	GrpGeom->Lock();

	for(const auto& it : CEffect) {
		const auto *ce = &it;
		switch(ce->type){
			case(CEFC_STAR):
				for(uint8_t k = 0; k < 4; k++) {
					r = ce->r - k*7;
					if(r < 0) continue;
					GrpGeom->SetColor({ 5u, (k + 2u), (k + 2u) });
					for(j=0; j<5; j++){
						x1 = ce->x + cosl(ce->d + dtable[k]*ce->count/10 + j*256/5, r);
						y1 = ce->y + sinl(ce->d + dtable[k]*ce->count/10 + j*256/5, r);
						x2 = ce->x + cosl(ce->d + dtable[k]*ce->count/10 + (j+2)*256/5, r);
						y2 = ce->y + sinl(ce->d + dtable[k]*ce->count/10 + (j+2)*256/5, r);
						GrpGeom->DrawLine(x1, y1, x2, y2);
					}
				}
			break;

			case(CEFC_CIRCLE1):		// 集
				for(uint8_t k = 0; k < 4; k++) {
					r = ce->r - max(2, (k * ce->r)/8);
					if(r < 0) continue;
					GrpGeom->SetColor({ 5u, (k + 2u), (k + 2u) });
					GeomCircle({ ce->x, ce->y }, r);
				}
			break;

			case(CEFC_CIRCLE2):		// 離
				for(uint8_t k = 0; k < 4; k++) {
					r = ce->r - max(2, (k * ce->r)/12);
					//r = ce->r - max(2, (k * (600-ce->r))/16);
					if(r < 0) continue;
					GrpGeom->SetColor({ 5u, (k + 2u), (k + 2u) });
					GeomCircle({ ce->x, ce->y }, r);
				}
			break;
		}
	}

	GrpGeom->Unlock();
}

// 円エフェクトをセットする //
void CEffectSet(int x, int y, uint8_t type)
{
	auto ce = std::ranges::find_if(CEffect, [](const auto& ce) {
		return (ce.type == CEFC_NONE);
	});
	if(ce == std::end(CEffect)) {
		return;
	}

	ce->x     = x >> 6;
	ce->y     = y >> 6;
	ce->type  = type;
	ce->count = ce->d = 0;

	switch(type){
		case(CEFC_STAR):
			Snd_SEPlay(SOUND_ID_TAMEFAST);
			ce->rmax = ce->r = 400;
		break;

		case(CEFC_CIRCLE1):		// 集
			ce->rmax = ce->r = 600+50;
		break;

		case(CEFC_CIRCLE2):		// 離
			ce->rmax = 900-100;
			ce->r    = 0;
		break;

		default:
		break;
	}
}

void MTitleInit(void)
{
	MTitleRect = TextObj.Register({ ((X_MAX + 1) - X_MIN), 20 });
}

// エフェクトの初期化を行う //
void SEffectInit(void)
{
	for(auto& it : SEffect) {
		//memset(SEffect+i,0,sizeof(SEFFECT_DATA));
		it.cmd = SEFC_NONE;
	}
}

// 文字列系エフェクト //
void StringEffect(int x, int y, const char *s)
{
	int		i,j,len;

	len = strlen(s);
	for(i=j=0;i<len;i++){
		while(SEffect[j].cmd!=SEFC_NONE){
			j++;
			if(j>=SEFFECT_MAX) return;
		}
		SEffect[j].c    = s[i];
		SEffect[j].x    = (x + (i<<4) + 512)<<6;
		SEffect[j].y    = (y)<<6;
		SEffect[j].vx   = (-20)<<6;
		SEffect[j].vy   = (0)<<6;
		SEffect[j].cmd  = SEFC_STR1;
		SEffect[j].time = 26;
	}
}

// 得点表示エフェクト //
void StringEffect2(int x, int y, uint32_t point)
{
	for(auto& it : SEffect) {
		if(it.cmd != SEFC_NONE) {
			continue;
		}

		it.point = point;
		it.x     = x;
		it.y     = y;
		it.vx    = 0;
		it.vy    = ((-64 * 3) + 32);
		it.cmd   = SEFC_STR2;
		it.time  = 90;
		return;
	}
}

// ゲームオーバーの表示 //
void StringEffect3(void)
{
	for(auto& it : SEffect) {
		if(it.cmd != SEFC_NONE) {
			continue;
		}

		it.x     = GX_MID;
		it.y     = (GY_MID - (64 * (60 + 40)));
		it.vx    = 0;
		it.vy    = 0;
		it.cmd   = SEFC_GAMEOVER;
		it.time  = 120-35;//100;
		return;
	}
}

// 曲名の表示 //
void SetMusicTitle(int y, Narrow::string_view s)
{
	// 空きバッファ検索 //
	auto e = std::ranges::find_if(SEffect, [](const auto& e) {
		return (e.cmd == SEFC_NONE);
	});
	if(e == std::end(SEffect)) {
		return;
	}

	// Print the ♪ separately, since it can be combined with a music title in
	// any encoding.
	const std::array<Narrow::string_view, 2> strs = { "♪ ", s };
	const auto extent = DrawGrdFont(MTitleRect, strs);

	// 曲のタイトルが長すぎる場合、どうしましょう？ //
	auto x = (std::max)((640 - 128 - 32 - extent.w), 128);

	e->cmd  = SEFC_MTITLE1;
	e->x    = (x << 6); // + ((64 * 2) * 16);
	e->y    = (y << 6);
	e->time = (64 * 2);
	e->vx   = extent.w;
	e->vy   = extent.h;
}

// エフェクトを動かす(仕様変更の可能性があります) //
void SEffectMove(void)
{
	BYTE			deg;

	for(auto& it : SEffect) {
		auto *e = &it;
		switch(e->cmd){
			case(SEFC_STR1):
				e->x += e->vx;
				e->y += e->vy;
				if(e->time==0){
					e->cmd  = SEFC_STR1_2;
					e->time = 256;//128;
				}
			break;

			case(SEFC_STR1_3):
				e->x += e->vx;
				e->y += (e->vy+=16);
				if(e->time==0) e->cmd = SEFC_NONE;
			break;

			case(SEFC_STR1_2):
				if(e->time==0){
					deg = 128 + rnd()%128;
					e->cmd  = SEFC_STR1_3;
					e->time = 64;
					e->vx   = cosl(deg,10*64);
					e->vy   = sinl(deg,10*64);
				}
			break;

			case(SEFC_STR2):
				if(e->time==0) e->cmd = SEFC_NONE;
				e->x += e->vx;
				e->y += (e->vy+=3);
			break;

			case(SEFC_GAMEOVER):
				if(e->time==0){
					e->cmd  = SEFC_GAMEOVER2;
					e->time = 35;
				}
			break;

			case(SEFC_MTITLE1):		// 曲名出現
				//e->x -= 16;
				if(e->time==0){
					e->cmd  = SEFC_MTITLE2;
					e->time = 64*4;
				}
			break;

			case(SEFC_MTITLE2):		// 曲名停止
				if(e->time==0){
					e->cmd  = SEFC_MTITLE3;
					e->time = 64*2;
				}
			break;

			case(SEFC_MTITLE3):		// 曲名消去
				e->x += 64;
				if(e->time==0){
					e->cmd = SEFC_NONE;
				}
			break;

			case(SEFC_NONE):default:
			break;
		}
		e->time--;
	}
}

// エフェクトを描画する(仕様変更の可能性があります) //
void SEffectDraw(void)
{
	int j, k;
	int				x,y;
	int				temp;
	BYTE			degx = 0;
	PIXEL_LTWH src;
	char			buf[20];

	for(const auto& it : SEffect) {
		const auto *e = &it;
		switch(e->cmd){
			case(SEFC_STR1):case(SEFC_STR1_2):case(SEFC_STR1_3):
				GrpPutc(e->x>>6,e->y>>6,e->c);
			break;

			case(SEFC_STR2):
				sprintf(buf, "%u", e->point);
				GrpPutScore(e->x>>6,e->y>>6,buf);
			break;

			case(SEFC_GAMEOVER):
				strcpy(buf,"GAME OVER");
				for(j=0;j<9;j++){
					// *37 //
					const auto angle = Cast::down<uint8_t>(
						(e->time * 3) + (j * 26)
					);
					x = (e->x >> 6) + cosl(angle, e->time * 4);
					y = (e->y >> 6) + sinl(angle, e->time * 4);
					GrpPutc(x,y,buf[j]);
				}
			break;

			case(SEFC_GAMEOVER2):
				x = (e->x >> 6) + 8;
				y = (e->y >> 6) + 8;
				j = (35 - e->time) / 2;
				GrpGeom->Lock();
				GrpGeom->SetColor({ 0, 0, 0 });
				GrpGeom->SetAlphaNorm(Cast::down<uint8_t>((35 - e->time) * 3));
				GrpGeom->DrawBoxA((x - 170), (y - j), (x + 170), (y + j));
				GrpGeom->Unlock();

				strcpy(buf,"GAME OVER");
				for(j=0;j<9;j++){
					// *37 //
					x = (e->x>>6) + (j-4)*(35-e->time);
					y = (e->y>>6);
					GrpPutc(x,y,buf[j]);
				}
			break;

			case(SEFC_MTITLE3): {
				degx = Cast::down<uint8_t>((64 * 2) - e->time);
				for(j=0;j<e->vx;j++){
					src = { j, 0, 1, e->vy };
					temp = sinl(degx,100);
					const auto y = (
						(e->y >> 6) - sinl((degx + j), degx /*40*/)
					);
					for(k=0;k<2;k++){
						const auto x = (
							(e->x >> 6) + sinl((degx + (j / 2)), temp) + j + k
						);
						TextObj.Blit({ x, y }, MTitleRect, src);
					}
				}
			}
			break;

			case(SEFC_MTITLE1): {
				degx = Cast::down<uint8_t>(e->time);
				for(j=0;j<e->vx;j++){
					src = { j, 0, 1, e->vy };
					temp = sinl(degx,160);
					const auto y = (
						(e->y >> 6) - sinl((degx + j), degx /*40*/)
					);
					for(k=0;k<2;k++){
						const auto x = (
							(e->x >> 6) + sinl((degx + (j / 2)), temp) + j + k
						);
						TextObj.Blit({ x, y }, MTitleRect, src);
					}
				}
			}
			break;

			case(SEFC_MTITLE2): {
				GrpGeom->Lock();
				GrpGeom->SetColor({ 0, 0, 0 });
				GrpGeom->SetAlphaNorm(
					(sinl(Cast::down<uint8_t>(e->time - 32), 80) + 80)
				);
				for(j=0;j<16;j++){
					temp = sinl(128+j*16,16);
					GrpGeom->DrawBoxA(
						((e->x >> 6) + temp - 16),
						((e->y >> 6) + j),
						((X_MAX - 16) - temp),
						((e->y >> 6) + j + 1)
					);
				}
				GrpGeom->Unlock();
				src = { 0, 0, e->vx, e->vy };
				TextObj.Blit({ (e->x >> 6), (e->y >> 6) }, MTitleRect, src);
			}
			break;

			case(SEFC_NONE):default:
			break;
		}
	}
}

// 画面全体に対するエフェクトの初期化 //
void ScreenEffectInit(void)
{
	ScreenInfo.cmd   = SCNEFC_NONE;
	ScreenInfo.count = 0;

	GrpBackend_SetClip(PLAYFIELD_CLIP);
}

// 画面全体に対するエフェクトをセットする //
void ScreenEffectSet(uint8_t cmd)
{
	//if(ScreenInfo.cmd != SCNEFC_NONE) return;

	ScreenInfo.cmd   = cmd;
	ScreenInfo.count = 0;

	switch(cmd){
		case(SCNEFC_CFADEIN):	// 円形フェードイン
		case(SCNEFC_CFADEOUT):	// 円形フェードアウト
		break;

		case(SCNEFC_WHITEIN):	// ホワイトイン
		case(SCNEFC_WHITEOUT):	// ホワイトアウト
		break;

		default:	// ばぐばぐ
			ScreenInfo.cmd = SCNEFC_NONE;	// 一応エフェクトを切っておく
		break;
	}
}

// 画面全体に対するエフェクトを動かす //
void ScreenEffectMove(void)
{
	switch(ScreenInfo.cmd){
		case(SCNEFC_CFADEIN):	// 円形フェードイン
			ScreenInfo.count+=10;
			if(ScreenInfo.count>600) ScreenInfo.cmd = SCNEFC_NONE;
		break;

		case(SCNEFC_CFADEOUT):	// 円形フェードアウト
			ScreenInfo.count+=10;
			if(ScreenInfo.count>600){
				ScreenInfo.count = 600;
				//ScreenInfo.cmd = SCNEFC_NONE;
			}
		break;

		case(SCNEFC_WHITEIN):	// ホワイトイン
			ScreenInfo.count+=10;
			if(ScreenInfo.count>=160){
				//ScreenInfo.cmd   = SCNEFC_WHITEOUT;
				//ScreenInfo.count = 0;
				ScreenInfo.count = 150;
			}
		break;

		case(SCNEFC_WHITEOUT):	// ホワイトアウト
			ScreenInfo.count+=10;
			if(ScreenInfo.count>=160) ScreenInfo.cmd = SCNEFC_NONE;
		break;

		default:	// ばぐばぐ
		break;
	}
}

// 画面全体に対するエフェクトを描画する //
void ScreenEffectDraw(void)
{
	int		i, j;
	PIXEL_LTRB	src;

	switch(ScreenInfo.cmd){
		case(SCNEFC_CFADEIN):	// 円形フェードイン
			CircleFadeOut(X_MID,Y_MID,ScreenInfo.count);
		break;

		case(SCNEFC_CFADEOUT):	// 円形フェードアウト
			CircleFadeOut(X_MID,Y_MID,400-ScreenInfo.count);
		break;

		case(SCNEFC_WHITEIN):	// ホワイトイン
			src = PIXEL_LTWH{
				((15 - (Cast::sign<int>(ScreenInfo.count) / 10)) * 16),
				(128 + 16),
				16,
				16
			};
			for(i=128; i<640-128; i+=16){
				for(j=0; j<480; j+=16){
					GrpSurface_Blit({ i, j }, SURFACE_ID::SYSTEM, src);
				}
			}
		break;

		case(SCNEFC_WHITEOUT):	// ホワイトアウト
			src = PIXEL_LTWH{
				((Cast::sign<int>(ScreenInfo.count) / 10) * 16),
				(128 + 16),
				16,
				16
			};
			for(i=128; i<640-128; i+=16){
				for(j=0; j<480; j+=16){
					GrpSurface_Blit({ i, j }, SURFACE_ID::SYSTEM, src);
				}
			}
		break;

		default:	// ばぐばぐ
		break;
	}
}

// 円形フェードサポート関数 //
extern void CircleFadeOut(int x, int y, int r)
{
	PIXEL_LTRB	src;
	int temp;

	if(r<0) r=0;


	for(auto i = 0; i < GRP_RES.w; i += 16) {
		for(auto j = 0; j < GRP_RES.h; j += 16) {
			temp = isqrt((i-x)*(i-x)+(j-y)*(j-y));
			if(temp<r && r-temp<8*16 && temp>=0){
				temp = (r-temp)>>3;
				//temp = (r-temp)>>4;
				//src = PIXEL_LTWH{ ((temp << 4) + 256), 104, 16, 16 };
				src = PIXEL_LTWH{ (temp << 4), 128, 16, 16 };
				GrpSurface_Blit({ i, j }, SURFACE_ID::SYSTEM, src);
			}
			else if(temp>=r) {
				//src = PIXEL_LTWH{ 256, 104, 16, 16 };
				src = PIXEL_LTWH{ 0, 128, 16, 16 };
				GrpSurface_Blit({ i, j }, SURFACE_ID::SYSTEM, src);
			}
		}
	}

	if(r != 0){
		const WINDOW_LTRB clip = {
			std::clamp((x - r), X_MIN, (X_MAX + 1)),
			(std::max)((y - r), Y_MIN),
			std::clamp((x + r + 1), X_MIN, (X_MAX + 1)),
			(std::min)((y + r + 1), (Y_MAX + 1))
		};
		GrpBackend_SetClip(clip);
	}
	else{
		GrpBackend_SetClip({ X_MID, Y_MID, X_MID, Y_MID });
	}
}

// ロックオン配列を初期化 //
void ObjectLockOnInit(void)
{
	for(auto& it : LockInfo) {
		//memset(LockInfo+i,0,sizeof(LOCKON_INFO));
		it.state = LOCKON_NONE;
	}
}

// 何かをロックオンする //
void ObjectLockOn(int *x, int *y, int wx64, int hx64)
{
	auto l = std::ranges::find_if(LockInfo, [](const auto& l) {
		return (l.state == LOCKON_NONE);
	});
	if(l == std::end(LockInfo)) {
		return;
	}

	l->x = x;
	l->y = y;

	l->width  = (wx64<<2);
	l->height = (hx64<<2);

	l->vx = -(wx64*4/30);
	l->vy = -(hx64*4/30);
	l->count = 30;

	l->state = LOCKON_01;
	// Snd_SEPlay(SOUND_ID_SELECT);
}

// ロックオンアニメーション動作 //
void ObjectLockMove(void)
{
	for(auto& it : LockInfo) {
		auto *l = &it;
		switch(l->state){
			case(LOCKON_01):
				l->count--;
				l->width  += l->vx;
				l->height += l->vy;
				if(l->count==0){
					l->state = LOCKON_NONE;//LOCKON_02;
					l->count = 30;//120;
				}
			break;
			case(LOCKON_02):
				l->count--;
				if(l->count==0){
					l->state = LOCKON_NONE;//LOCKON_03;
					l->count = 30;
				}
			break;
			case(LOCKON_03):
				l->count--;
				l->width  -= l->vx;
				l->height -= l->vy;
				if(l->count==0){
					l->state = LOCKON_NONE;
				}
			break;
		}
	}
}

// ロックオン枠描画 //
void ObjectLockDraw(void)
{
	for(const auto& it : LockInfo) {
		const auto *l = &it;
		if(l->state!=LOCKON_NONE){
			GrpSurface_Blit(
				{ ((*l->x >> 6) - 8), ((*l->y >> 6) - 8) },
				SURFACE_ID::SYSTEM,
				{ 208, 80, 272, 96 }
			);

			GrpSurface_Blit(
				{
					(((*l->x - l->width) >> 6) - 4),
					(((*l->y - l->height) >> 6) - 4),
				},
				SURFACE_ID::SYSTEM,
				{ 280, 88, 288, 96 }
			);
			GrpSurface_Blit(
				{
					(((*l->x + l->width) >> 6) - 4),
					(((*l->y + l->height) >> 6) - 4),
				},
				SURFACE_ID::SYSTEM,
				{ 272, 80, 280, 88 }
			);

			GrpSurface_Blit(
				{
					(((*l->x - l->width) >> 6) - 4),
					(((*l->y + l->height) >> 6) - 4),
				},
				SURFACE_ID::SYSTEM,
				{ 280, 80, 288, 88 }
			);
			GrpSurface_Blit(
				{
					(((*l->x + l->width) >> 6) - 4),
					(((*l->y - l->height) >> 6) - 4),
				},
				SURFACE_ID::SYSTEM,
				{ 272, 88, 280, 96 }
			);
		}
	}
}



#include "game/midi.h"
// スペアナ描画 //
void GrpDrawSpect(int x, int y)
{
	int		i,j,temp,temp2;
	WORD	ftable[128+8+8],ftable2[128];

	static WORD ftable3[128+8+8];
	static BYTE ftable3flag;

	constexpr PIXEL_LTRB src = {
		(16 * 16), 0, ((16 * 16) + (8 * 21)), 8
	}; // ,,,8*4

	ftable3flag = (ftable3flag+1)%5;

	for(i=0;i<128;i++){
		temp = temp2 = 0;
		for(j=0;j<16;j++){
			temp += Mid_PlayTable[j][i];
			temp2 += (Mid_PlayTable[j][i] ? 1 : 0);
			if(Mid_PlayTable[j][i]!=0) Mid_PlayTable[j][i]-=((Mid_PlayTable[j][i]>>3)+1);//4
			//if(Mid_PlayTable[j][i]) Mid_PlayTable[j][i]-=(Mid_PlayTable[j][i]>>3)+1;
		}
		if(temp2==0) temp2 = 1;
		ftable2[i] = temp/temp2;
	}

	for(i=0;i<128+8+8;i++){
		ftable[i]  = ((i>=8 && i<=128+7) ? ftable2[i-8] : 0)   *(256-sinm(0))/256;

		ftable[i] += ((i>=9  && i<=128+8 ) ? ftable2[i-9 ] : 0)*(256-sinm( 8))/256;
		ftable[i] += ((i>=10 && i<=128+9 ) ? ftable2[i-10] : 0)*(256-sinm(16))/256;
		ftable[i] += ((i>=11 && i<=128+10) ? ftable2[i-11] : 0)*(256-sinm(24))/256;
		ftable[i] += ((i>=12 && i<=128+11) ? ftable2[i-12] : 0)*(256-sinm(32))/256;
		ftable[i] += ((i>=13 && i<=128+12) ? ftable2[i-13] : 0)*(256-sinm(40))/256;
		ftable[i] += ((i>=14 && i<=128+13) ? ftable2[i-14] : 0)*(256-sinm(48))/256;
		ftable[i] += ((i>=15 && i<=128+14) ? ftable2[i-15] : 0)*(256-sinm(56))/256;
		ftable[i] += ((i>=16 && i<=128+15) ? ftable2[i-16] : 0)*(256-sinm(63))/256;

		ftable[i] += ((i<=128+6 && i>=7) ? ftable2[i-7] : 0) *(256-sinm( 8))/256;
		ftable[i] += ((i<=128+5 && i>=6) ? ftable2[i-6] : 0) *(256-sinm(16))/256;
		ftable[i] += ((i<=128+4 && i>=5) ? ftable2[i-5] : 0) *(256-sinm(24))/256;
		ftable[i] += ((i<=128+3 && i>=4) ? ftable2[i-4] : 0) *(256-sinm(32))/256;
		ftable[i] += ((i<=128+2 && i>=3) ? ftable2[i-3] : 0) *(256-sinm(40))/256;
		ftable[i] += ((i<=128+1 && i>=2) ? ftable2[i-2] : 0) *(256-sinm(48))/256;
		ftable[i] += ((i<=128+0 && i>=1) ? ftable2[i-1] : 0) *(256-sinm(56))/256;
		ftable[i] += ((i<=128-1 && i>=0) ? ftable2[i-0] : 0) *(256-sinm(63))/256;

		ftable[i]>>=3;

		if(ftable3[i]<ftable[i])
			ftable3[i] = ftable[i];
		else if((!ftable3flag) && ftable3[i])
			ftable3[i]--;
	}

	// GrpSurface_Blit({ (SPECT_X - 7), SPECT_Y }, SURFACE_ID::SYSTEM, src);

	GrpGeom->Lock();

	if(auto *gp = GrpGeom_Poly()) {
		for(i=0;i<128+8+8;i++){
			//WORD c2 = 0;	//5
			constexpr RGB c1 = { 200,   0, 0 };
			constexpr RGB c2 = { 250, 250, 0 };
			gp->DrawGrdLineEx((i + x), (y - (ftable[i] * 2)), c1, y, c2);
		}
	} else if(auto *gf = GrpGeom_FB()) {
		gf->SetColor({ 4, 2, 1 });
		for(i=0;i<128+8+8;i++){
			//WORD c2 = 0;	//5
			if(ftable[i]) {
				gf->DrawLine((i + x), (y - (ftable[i] * 2)), (i + x), y);
			}
		}
	}

	GrpGeom->Unlock();
}

// 押されているところを表示 //
void GrpDrawNote(void)
{
	// 0123456789ab (Mod c)
	// o#o#oo#o#o#o
	// o o oo o o o

	constexpr PIXEL_LTRB src[12] = {
		{ 0,464, 3,474},		// しろ
		{ 0,456, 3,461},		// 黒

		{ 4,464, 7,474},		// しろ
		{ 0,456, 3,461},		// 黒

		{ 8,464,11,474},		// しろ
		{12,464,15,474},		// しろ

		{ 0,456, 3,461},		// 黒
		{16,464,19,474},		// しろ

		{ 0,456, 3,461},		// 黒
		{20,464,23,474},		// しろ

		{ 0,456, 3,461},		// 黒
		{24,464,27,474},		// しろ
	};

	constexpr PIXEL_LTRB src2[12] = {
		{ 0,464-24, 3,474-24},		// しろ
		{ 0,456-24, 3,461-24},		// 黒

		{ 4,464-24, 7,474-24},		// しろ
		{ 0,456-24, 3,461-24},		// 黒

		{ 8,464-24,11,474-24},		// しろ
		{12,464-24,15,474-24},		// しろ

		{ 0,456-24, 3,461-24},		// 黒
		{16,464-24,19,474-24},		// しろ

		{ 0,456-24, 3,461-24},		// 黒
		{20,464-24,23,474-24},		// しろ

		{ 0,456-24, 3,461-24},		// 黒
		{24,464-24,27,474-24},		// しろ
	};

	constexpr int destX[12] = {
		0,		// しろ
		2,		// 黒

		4,		// しろ
		6,		// 黒

		8,		// しろ
		12,		// しろ

		14,		// 黒
		16,		// しろ

		18,		// 黒
		20,		// しろ

		22,		// 黒
		24,		// しろ
	};

	int		Track,NoteNo,x,y,LevelSum,num;
	PIXEL_LTRB	rc;

	for(Track=0;Track<16;Track++){
		const auto top = (22 + (Track * 24));
		const auto pan = (Cast::sign<int8_t>(Mid_PanpodTable[Track]) - 64);
		GrpPutMidNum( 50, top, Mid_VolumeTable[Track]);
		GrpPutMidNum(125, top, Mid_ExpressionTable[Track]);
		GrpPutMidNum(181, top, pan);

		LevelSum = num = 0;
		for(NoteNo=0;NoteNo<128;NoteNo++){
			if(Mid_NoteWTable[Track][NoteNo]){
				x = 40 + destX[NoteNo%12] + (NoteNo/12)*28;
				y =  9 + Track*24;
				GrpSurface_Blit({ x, y }, SURFACE_ID::MUSIC, src[NoteNo % 12]);
				Mid_NoteWTable[Track][NoteNo]--;
			}

			if(Mid_NoteTable[Track][NoteNo]){
				x = 40 + destX[NoteNo%12] + (NoteNo/12)*28;
				y =  9 + Track*24;
				GrpSurface_Blit({ x, y }, SURFACE_ID::MUSIC, src[NoteNo % 12]);
			}
			if(Mid_PlayTable2[Track][NoteNo]){
				LevelSum += (Mid_PlayTable2[Track][NoteNo]);
				//if(Mid_PlayTable2[Track][NoteNo]>128) Mid_PlayTable2[Track][NoteNo]=128;
				Mid_PlayTable2[Track][NoteNo] -= max(Mid_PlayTable2[Track][NoteNo]/50,1);
				num++;
			}
		}

		if(num){
			rc = PIXEL_LTWH{ 80, 456, (std::min)((LevelSum / num), 96), 5 };
			GrpSurface_Blit(
				{ 240, (22 + (Track * 24)) }, SURFACE_ID::MUSIC, rc
			);
		}
	}
}


bool bEnableWarnEfc = false;
WORD WarnEfcTime = 0;

// ワーニングの初期化 //
void WarningEffectInit(void)
{
	bEnableWarnEfc = false;
}

// ワーニングの発動！！ //
void WarningEffectSet(void)
{
	bEnableWarnEfc = true;
	WarnEfcTime    = 0;
	InitWarning();
}

// ワーニングの動作 //
void WarningEffectMove(void)
{
	if(!bEnableWarnEfc) return;

	if(WarnEfcTime<64+128){//64+256){
		MoveWarning(Cast::down<uint8_t>(WarnEfcTime));
	}
	else{
		MoveWarningR(-1);
	}

	if((WarnEfcTime++) == (256 + 10)) {
		bEnableWarnEfc = false;
	}
}

// ワーニングの描画 //
void WarningEffectDraw(void)
{
	int		r;

	if(!bEnableWarnEfc) return;

	if(WarnEfcTime<256-20)
		DrawWarning();

	if(WarnEfcTime>256-40){
		GrpGeom->Lock();

		r = (WarnEfcTime-(256-40))*3;
		GrpGeom->SetColor({ 1, 1, 5 });	GeomCircle({ 320, 100 }, (r -= 4));
		GrpGeom->SetColor({ 2, 2, 5 });	GeomCircle({ 320, 100 }, (r -= 4));
		GrpGeom->SetColor({ 3, 3, 5 });	GeomCircle({ 320, 100 }, (r -= 6));
		GrpGeom->SetColor({ 4, 4, 5 });	GeomCircle({ 320, 100 }, (r -= 6));
		GrpGeom->SetColor({ 5, 5, 5 });	GeomCircle({ 320, 100 }, (r -= 8));

		GrpGeom->Unlock();
	}
}
