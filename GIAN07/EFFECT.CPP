/*                                                                           */
/*   EFFECT.cpp   エフェクト管理                                             */
/*                                                                           */
/*                                                                           */

#include "EFFECT.H"
#include "GEOMETRY.H"
#include "ssg/Sound.h"
#include "ssg/internal/Effect3D.hpp"
#include "hatoyama/engine/snd.h"
#include "hatoyama/engine/text_backend.h"
#include "hatoyama/logic/cast.h"
#include "hatoyama/logic/ut_math.h"


CIRCLE_EFC_DATA		CEffect[CIRCLE_EFC_MAX];
LOCKON_INFO			LockInfo[LOCKON_MAX];
SCREENEFC_INFO ScreenInfo;


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
	static constexpr uint8_t dtable[4] = { 0, 1, 3, 7 };

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



/// Warning
/// -------

typedef struct tagLineList3D{
	std::span<const WORLD_POINT>	p;	/* 頂点の座標         */

	uint8_t	DegX, DegY, DegZ;	/* 各軸に対する回転角 */
} LineList3D;

template <size_t N> consteval auto TransformLineList(
	PIXEL_POINT center, std::array<PIXEL_POINT, N>&& in
)
{
	std::array<WORLD_POINT, N> ret;
	for(size_t i = 0; auto& p : ret) {
		p = (in[i++] - center);
	}
	return ret;
}

constinit auto PList_W = TransformLineList<11>({ 192, 39 }, {{
	{  0, 15 }, { 15, 66 }, { 32, 47 }, { 48, 66 },
	{ 63, 14 }, { 52, 11 }, { 42, 38 }, { 32, 26 },
	{ 21, 38 }, { 11, 10 }, {  0, 15 },
}});

constinit auto PList_A1 = TransformLineList<8>({ 192, 39 }, {{
	{  96, 12 }, {  66, 61 }, {  75, 67 }, { 83, 56 },
	{ 107, 56 }, { 115, 67 }, { 125, 61 }, { 96, 12 },
}});

constinit auto PList_A2 = TransformLineList<4>({ 192, 39 }, {{
	{ 96, 34 }, { 90, 44 }, { 101, 44 }, { 96, 34 },
}});

constinit auto PList_R = TransformLineList<15-1>({ 192, 39 }, {{
	{ 132, 14 }, { 132, 64 }, { 145, 64 }, { 145, 27 },
	{ 164, 27 },

	//{ 150, 41 }, { 150, 43 },
	{ 150, 42 },

	{ 171, 66 },
	{ 173, 66 }, { 181, 57 }, { 167, 43 }, { 180, 29 },
	{ 180, 27 }, { 170, 14 }, { 132, 14 },
}});

constinit auto PList_N1 = TransformLineList<9>({ 192, 39 }, {{
	{ 189, 12 }, { 189, 64 }, { 201, 64 }, { 201, 40 },
	{ 239, 66 }, { 239, 14 }, { 227, 14 }, { 227, 38 },
	{ 189, 12 },
}});

constinit auto PList_N2 = TransformLineList<9>({ (192 - (296 - 215)), 39 }, {{
	{ 189, 12 }, { 189, 64 }, { 201, 64 }, { 201, 40 },
	{ 239, 66 }, { 239, 14 }, { 227, 14 }, { 227, 38 },
	{ 189, 12 },
}});

constinit auto PList_I = TransformLineList<5>({ 192, 39 }, {{
	{ 248, 14 }, { 248, 64 }, { 262, 64 }, { 262, 14 },
	{ 248, 14 },
}});

constinit auto PList_G = TransformLineList<17>({ 192, 39 }, {{
	{ 354, 11 }, { 328, 22 }, { 328, 57 }, { 354, 68 },
	{ 380, 59 }, { 380, 34 }, { 355, 34 }, { 354, 45 },
	{ 367, 46 }, { 367, 51 }, { 355, 55 }, { 342, 50 },
	{ 342, 29 }, { 354, 24 }, { 372, 30 }, { 377, 19 },
	{ 354, 11 },
}});

LineList3D Warning[8] = {
	{ PList_W },
	{ PList_A1 },
	{ PList_A2 },
	{ PList_R },
	{ PList_N1 },
	{ PList_I },
	{ PList_N2 },
	{ PList_G },
};

bool bEnableWarnEfc = false;
uint16_t WarnEfcTime = 0;


static void RollPoint(Point3D *p, uint8_t dx, uint8_t dy, uint8_t dz)
{
	Point3D		temp;

	temp.y = p->y;
	temp.z = p->z;
	p->y = cosl(dx,temp.y) - sinl(dx,temp.z);
	p->z = sinl(dx,temp.y) + cosl(dx,temp.z);

	temp.x = p->x;
	temp.z = p->z;
	p->x =   cosl(dy,temp.x) + sinl(dy,temp.z);
	p->z = - sinl(dy,temp.x) + cosl(dy,temp.z);

	temp.x = p->x;
	temp.y = p->y;
	p->x = cosl(dz,temp.x) - sinl(dz,temp.y);
	p->y = sinl(dz,temp.x) + cosl(dz,temp.y);
}

static void MoveWarning(uint8_t count)
{
	for(auto& llist : Warning) {
		llist.DegX = ((count < 64) ? ((64 - count) * 2) : 0);
		llist.DegY = ((count < 64) ? ((64 - count) * 1) : 0);
		llist.DegZ = ((count < 64) ? ((64 - count) * 4) : 0);
	}
}

static void MoveWarningR(char count)
{
	if(count==0) return;

	for(auto& llist : Warning) {
		llist.DegX += (count * 2);
		llist.DegY += (count * 1);
		llist.DegZ += (count * 4);
	}
}

static void DrawLineList3D(std::span<const LineList3D> w)
{
	const auto roll = [](const LineList3D& llist, const WORLD_POINT& point) {
		Point3D temp = { point.x, point.y, 0 };
		RollPoint(&temp, llist.DegX, llist.DegY, llist.DegZ);
		return WORLD_POINT{ &temp.x, &temp.y };
	};

	for(const auto& llist : w) {
		WORLD_POINT line_p[2] = { roll(llist, llist.p.front()) };
		for(const auto& point : (llist.p | std::views::drop(1))) {
			line_p[1] = roll(llist, point);
			const auto p1 = (PIXEL_POINT{ 320, 100 } + line_p[0].ToPixel());
			const auto p2 = (PIXEL_POINT{ 320, 100 } + line_p[1].ToPixel());
			GrpGeom->DrawLine(p1.x, p1.y, p2.x, p2.y);
			std::swap(line_p[1], line_p[0]);
		}
	}
}

void DrawWarning(void)
{
	static const PIXEL_LTRB src = { 0, (152 + 16), 384, (232 + 16) };
	int	st,det;
	static int count;

	count+=8;

	if(Warning[0].DegX == 0) {
		GrpGeom->Lock();
		GrpGeom->SetAlphaNorm(Cast::down_sign<uint8_t>(128 + sinl(count, 48)));
		GrpGeom->SetColor({ 5, 0, 0 });
		GrpGeom->DrawBoxA(129, (40 + 6), (128 + 384), (60 + 6));
		GrpGeom->DrawBoxA(
			129, (60 + (232 - 152) - 4), (128 + 384), (80 + (232 - 152) - 4)
		);
		GrpGeom->Unlock();
		GrpSurface_Blit({ (128 + 1), (60 + 1) }, SURFACE_ID::SYSTEM, src);
	}
	else{
		GrpGeom->Lock();

		if(Warning[0].DegX < 10) {
			st=0;
			det=0;
		} else if(Warning[0].DegX < 20) {
			st=-4;
			det=1;
		}
		else{
			//if(Warning[0].DegX < 40){
			st  = -8;
			det = 2;
		}

		constexpr auto w = std::span(Warning);
		GrpGeom->SetColor({ 1, 1, 5 });	MoveWarningR(st); 	DrawLineList3D(w);
		GrpGeom->SetColor({ 2, 2, 5 });	MoveWarningR(det);	DrawLineList3D(w);
		GrpGeom->SetColor({ 3, 3, 5 });	MoveWarningR(det);	DrawLineList3D(w);
		GrpGeom->SetColor({ 4, 4, 5 });	MoveWarningR(det);	DrawLineList3D(w);
		GrpGeom->SetColor({ 5, 5, 5 });	MoveWarningR(det);	DrawLineList3D(w);

		GrpGeom->Unlock();
	}
}


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
/// -------
