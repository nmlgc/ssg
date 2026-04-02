/*
 *   Rendering code for game logic entities
 *
 */

#include "GameDraw.h"
#include "BOSS.H"
#include "EnemyExCtrl.h"
#include "BOMBEFC.H"
#include "EFFECT3D.H"
#include "FONTUTY.H"
#include "GEOMETRY.H"
#include "GIAN.H"
#include "MAID.H"
#include "TAMA.H"
#include "hatoyama/logic/ut_math.h"

/// Bit lines
/// ---------

// ビット間のラインを描画する //
void BitLineDraw(const C_BIT& Bit)
{
	const auto BitData = Bit.Data();
	int				i, j, n;
	int				x1, x2, y1, y2;
	ENEMY_DATA		*RefTable[BIT_MAX*2];

	if(BitData.State == BITCMD_DISABLE) return;

	n = BitData.NumBits;
	if(n == 0) return;

	for(i=0, j=-1; i<n; i++){
		while(BitData.Bit[++j].pEnemy == nullptr) {
		}

		RefTable[i] = RefTable[i + n] = BitData.Bit[j].pEnemy;
	}

	GrpGeom->Lock();
	GrpGeom->SetColor({ 4, 4, 5 });

	for(i=0; i<n; i++){
		if(n >= 5) j = i + 2;
		else       j = i + 1;

		x1 = RefTable[i]->x >> 6;
		y1 = RefTable[i]->y >> 6;
		x2 = RefTable[j]->x >> 6;
		y2 = RefTable[j]->y >> 6;
		GrpGeom->DrawLine(x1, y1, x2, y2);
	}

	GrpGeom->Unlock();
}
/// ---------

/// Enemies
/// -------

static void _EnemyDraw(const C_ENEMY& Enemy, const ENEMY_DATA& e)
{
	constexpr auto sid = SURFACE_ID::ENEMY;

	// TODO: Remove once the structure itself uses WORLD_POINT.
	const WORLD_POINT center = { &e.x, &e.y };

	const auto& a = Enemy.AnimeSheet(e.anm_ptn);
	const auto topleft = center.ToPixel(a.size); // 座標セット //

	// 描画モード選択 //
	const auto& src = ((a.mode == ANM_DEG)
		? a.ptn[uint8_t(e.d - 64 + 8) >> 4]
		: a.ptn[e.anm_c]
	);
	if(GrpSurface_Blit({ topleft.x, topleft.y }, sid, src)) {
		if((e.anm_ptn != e.anm_ptnEx) && e.IsDamaged) {
			const auto& a = Enemy.AnimeSheet(e.anm_ptnEx);
			const auto topleft = center.ToPixel(a.size); // 座標セット //
			GrpSurface_Blit({ topleft.x, topleft.y }, sid, a.ptn[0]);
		}
	}
}

static void _EnemyDrawBomb(int x, int y, uint32_t count)
{
	PIXEL_LTRB	src;

	src.top    = 296;
	src.left   = (count/ENEMY_BOMB_SPD)*48;
	src.bottom = 296      +48;
	src.right  = src.left +48;

	x-=24;
	y-=24;

	GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src);
}

void enemy_draw(const C_ENEMY& Enemy)
{
	const auto storage = Enemy.Data();
	for(const auto ind : Enemy.Inds()) {
		const auto *e = &storage[ind];

		// 敵を描画する(クリッピング＆幅、高さ処理を追加すること) //
		const auto x = (e->x>>6);
		const auto y = (e->y>>6);
		if(e->flag==EF_BOMB){
			_EnemyDrawBomb(x,y,e->count);
			continue;
		}

		if(e->flag&EF_DRAW){
			_EnemyDraw(Enemy, *e);
		}
	}
}
/// -------

/// Bosses
/// ------

// ボスを描画する
void BossDraw(const C_BOSS& Boss, const C_ENEMY& Enemy, const MAID& Viv)
{
	constexpr auto sid = SURFACE_ID::ENEMY;
	int x, y;
	int			w,h,t;
	PIXEL_LTRB	wing;

	for(auto& it : Boss.Data()) {
		auto *b = &it;
		if(b->IsUsed){
			const auto *e = &(b->Edat);

			x = (e->x>>6);
			y = (e->y>>6);

			// 霊魂状態 //
			if(b->ExState == BEXST_SHILD2 && Viv.bomb_time && (e->flag&EF_DRAW)){
				wing = PIXEL_LTWH{
					(160 + (Cast::sign<int32_t>(e->count / 2) % 4) * 40),
					80,
					40,
					40
				};

				// pbg quirk: Blitted without clipping?! I'd consider this a
				// bug if it wasn't explicitly commented as such. Fine then...
				GrpBackend_SetClip(GRP_RES_RECT);

				// クリッピングなし
				GrpSurface_Blit({ (x - 20), (y - 20) }, sid, wing);

				GrpBackend_SetClip({ X_MIN, Y_MIN, (X_MAX + 1), (Y_MAX + 1) });
				continue;
			}

			// バリア状態 //
			if(b->ExState == BEXST_SHILD1 && Viv.bomb_time && (e->flag&EF_DRAW)){
				GrpGeom->Lock();
				for(uint8_t j = 0; j <= 5; j++) {
					GrpGeom->SetColor({ (5u - j), (5u - j), 5u });
					GeomCircle(
						{ x, y }, (sinl((e->count * 4), (30 + (j * 4))) + 80)
					);
				}
				GrpGeom->Unlock();
			}

			switch(b->ExState){
				case(BEXST_WING01):
					t = (b->ExCount-64-8)<<2;
					if(t<0) t=0;
					w = 64;
					h = 92;
					wing = {   0, 176, 128, 360 };
					GrpSurface_Blit({ (x - w - t), (y - h) }, sid, wing);
					wing = { 128, 176, 256, 360 };
					GrpSurface_Blit({ (x - w + t), (y - h) }, sid, wing);
				break;

				case(BEXST_WING02):
					w = 44;
					h = 52;
					wing = { 552,   0, 640, 104 };
					GrpSurface_Blit({ (x - w - 50), (y - h) }, sid, wing);
					wing = { 552, 104, 640, 208 };
					GrpSurface_Blit({ (x - w + 50), (y - h) }, sid, wing);
				break;
			}

			if(e->flag&EF_DRAW){
				_EnemyDraw(Enemy, *e);
			}
		}
	}
}
/// ------

/// Boss explosions
/// ---------------

// 爆発系エフェクトを描画する
void ExBombEfcDraw(const C_BOMBEFC& BombEfc)
{
	for(const auto& it : BombEfc.Data()) {
		if(!it.bIsUsed) {
			continue;
		}
		switch(it.type) {
		case EXBOMB_STD:
			// Graphic 48 * 48 //
			for(const auto& obj : it.Obj) {
				if(obj.d <= (7 * 2)) {
					const WINDOW_COORD x = ((obj.x >> 6) - 24);
					const WINDOW_COORD y = ((obj.y >> 6) - 24);
					const PIXEL_COORD dx = ((obj.d >> 1) * 48);
					const PIXEL_LTRB src = PIXEL_LTWH{ dx, 296, 48, 48 };
					GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src);
				}
			}
			break;

		default:
			return;
		}
	}
}
/// ---------------

/// 3D effects
/// ----------

// ３面高速星描画 //
void DrawStg3Star(const C_EFFECT3D& Effect3D)
{
	const auto S6Star = Effect3D.DataS6Star();
	int		i;

	for(i=0; i<S3STAR_MAX; i++){
		static const PIXEL_LTRB src = { (640 - 16), 0, 640, 16 };
		GrpSurface_Blit({ S6Star[i].x, S6Star[i].y }, SURFACE_ID::MAPCHIP, src);
	}
}


void DrawStg4Rock(const C_EFFECT3D& Effect3D)
{
	constexpr auto sid = SURFACE_ID::MAPCHIP;
	static const PIXEL_LTRB src[3] = {
		{  0, 224, 80, 288 },
		{  0, 288, 48, 336 },
		{ 48, 288, 80, 320 },
	};
	static const int dx[3] = { (80 / 2), (48 / 2), (32 / 2) };
	static const int dy[3] = { (64 / 2), (48 / 2), (32 / 2) };

	int x, y;

	for(const auto& it : Effect3D.DataRock()) {
		const auto *p = &it;
		x = (p->x + GX_MID)>>6;
		y = (p->y + GY_MID)>>6;
		GrpSurface_Blit(
			{ (x - dx[p->GrpID]), (y - dy[p->GrpID]) }, sid, src[p->GrpID]
		);
	}
}


// ６面ラスター描画 //
void DrawStg6Raster(const C_EFFECT3D& Effect3D)
{
	constexpr auto sid = SURFACE_ID::MAPCHIP;
	static const PIXEL_LTRB Target[3] = {
		{ 608, 272, 640, 352 },
		{ 592, 160, 640, 272 },
		{ 576,   0, 640, 160 },
	};

	int		i, j, h, w;
	int		x1, x2, dx, oy;

	const auto S6Star = Effect3D.DataS6Star();
	for(i = 0; i < S6STAR_MAX; i++) {
		static const PIXEL_LTRB src = { 624, 352, (624 + 16), (352 + 16) };
		GrpSurface_Blit({ S6Star[i].x, S6Star[i].y }, sid, src);
	}

	for(const auto& it : Effect3D.DataS6Ras()) {
		x1 = Target[it.type].left;
		x2 = Target[it.type].right;
		oy = Target[it.type].top;
		h = (Target[it.type].bottom - Target[it.type].top);
		w  = (x2 - x1)/2;
		for(j=0; j<h; j+=2){
			dx = sinl((it.deg + j), it.amp);
			const PIXEL_LTRB src = { x1, (j + oy), x2, (j + 2) };
			GrpSurface_Blit({ (it.x + dx - w), (it.y + j) }, sid, src);
		}
	}
}


static void ShiftRight6Bit(const Point3D *o, Point3D *p)
{
	p->x = (((p->x + o->x) >> 6) + 320);
	p->y = (((p->y + o->y) >> 6) + 240);
}

// 汎用３Ｄキューブ描画 //
static void __Draw3DCube(const Cube3D *c)
{
	// ３Ｄで透過色付きorアルファ比較ポリゴンを使えばもっともっと早いのだが... //
	// ８ビット対応のため、まぁ仕方が無いか... //

	int			x,y,z;
	int			l,l2;
	Point3D		p1,p2;
	Point3D		o;

	o  = c->p;
	l  = c->l;
	const uint8_t dx = c->d.dx;
	const uint8_t dy = c->d.dy;
	const uint8_t dz = c->d.dz;

	l2  = l;

	// GrpGeom->SetColor({ 1, 1, 2 });
	GrpGeom->SetColor({ 1, 1, 3 });
	for(x=-1; x<=1; x++){
		for(y=-1; y<=1; y++){
			p1.x = x * l;
			p1.y = y * l;
			p1.z =    -l2;
			Transform3D(&p1, dx, dy, dz);
			ShiftRight6Bit(&o,&p1);

			p2.x = x * l;
			p2.y = y * l;
			p2.z =     l2;
			Transform3D(&p2, dx, dy, dz);
			ShiftRight6Bit(&o,&p2);

			GrpGeom->DrawLine(p1.x, p1.y, p2.x, p2.y);
		}
	}

	GrpGeom->SetColor({ 0, 0, 3 });
	for(y=-1; y<=1; y++){
		for(z=-1; z<=1; z++){
			p1.x =    -l2;
			p1.y = y * l;
			p1.z = z * l;
			Transform3D(&p1, dx, dy, dz);
			ShiftRight6Bit(&o,&p1);

			p2.x =     l2;
			p2.y = y * l;
			p2.z = z * l;
			Transform3D(&p2, dx, dy, dz);
			ShiftRight6Bit(&o,&p2);

			GrpGeom->DrawLine(p1.x, p1.y, p2.x, p2.y);
		}
	}

	// GrpGeom->SetColor({ 1, 1, 3 });
	GrpGeom->SetColor({ 1, 1, 4 });
	for(x=-1; x<=1; x++){
		for(z=-1; z<=1; z++){
			p1.x = x * l;
			p1.y =    -l2;
			p1.z = z * l;
			Transform3D(&p1, dx, dy, dz);
			ShiftRight6Bit(&o,&p1);

			p2.x = x * l;
			p2.y =     l2;
			p2.z = z * l;
			Transform3D(&p2, dx, dy, dz);
			ShiftRight6Bit(&o,&p2);

			GrpGeom->DrawLine(p1.x, p1.y, p2.x, p2.y);
		}
	}
}

void Draw3DCube(const C_EFFECT3D& Effect3D)
{
	for(const auto& it : Effect3D.DataStar()) {
		static const PIXEL_LTWH rc = { 136, 272, 16, 24 };
		GrpSurface_Blit({ it.x, it.y }, SURFACE_ID::SYSTEM, rc);
	}

	GrpGeom->Lock();
	for(const auto& it : Effect3D.DataCube()) {
		__Draw3DCube(&it);
	}
	GrpGeom->Unlock();
}


void DrawEffectFakeECL(const C_EFFECT3D& Effect3D)
{
	const auto& WFLine = Effect3D.DataWFLine();
	PIXEL_LTRB	src;
	int		i,j;

	GrpGeom->Lock();

	// GrpGeom->SetColor({ 3, 2, 0 });	// 後半戦用
	GrpGeom->SetColor({ 0, 2, 0 });
	// GrpGeom->SetColor({ 0, 0, 3 });

	for(i=128-WFLine.ox/2; i<640-128; i+=32)
		GrpGeom->DrawLine(i, 0, i, 480);

	for(j=WFLine.oy/2; j<480; j+=32)
		GrpGeom->DrawLine(128, j, (640 - 128), j);


	// GrpGeom->SetColor({ 5, 3, 0 });	// 後半戦用
	GrpGeom->SetColor({ 0, 3, 0 });
	// GrpGeom->SetColor({ 0, 0, 4 });

	for(i=128-WFLine.ox; i<640-128; i+=64)
		GrpGeom->DrawLine(i, 0, i, 480);

	for(j=-WFLine.oy; j<480; j+=64)
		GrpGeom->DrawLine(128, j, (640 - 128), j);

	GrpGeom->Unlock();

	for(const auto& it : Effect3D.DataFakeECL()) {
		src = PIXEL_LTWH{it.SrcX, it.SrcY, 72, 16};
		GrpSurface_Blit({ (it.x >> 6), (it.y >> 6) }, SURFACE_ID::MAPCHIP, src);
	}

	src = { 0, 272, 416, 352 };
	GrpSurface_Blit({ 128, 400 }, SURFACE_ID::MAPCHIP, src);
}

/// ----------

/// Particles
/// ---------

void fragment_draw(FRAGMENT_DATA_CSPAN Fragment)
{
	for(const auto& f : Fragment) {
		if(f.count == 0) {
			continue;
		}

		int x, y;
		PIXEL_LTRB src;

		switch(f.cmd) {
		case(FRG_EVADE):
			x = (f.x >> 6) - 4;
			y = (f.y >> 6) - 4;
			src = PIXEL_LTWH{ (592 + (((24 - f.count) >> 2) << 3)), 8, 8, 8 };
			GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src);
			break;

		case(FRG_HIT):
			x = (f.x >> 6) - 4;
			y = (f.y >> 6) - 4;
			src = PIXEL_LTWH{
				(592 + (((24 - f.count) >> 2) << 3)), (8 + 8), 8, 8
			};
			GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src);
			break;

		case(FRG_SMOKE):
			x = (f.x >> 6) - 4;
			y = (f.y >> 6) - 4;
			src = PIXEL_LTWH{ (592 + (((24 - f.count) >> 2) << 3)), 0, 8, 8 };
			GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src);
			break;

		case(FRG_STAR1):
			x = (f.x >> 6) - 8;
			y = (f.y >> 6) - 8;
			src = PIXEL_LTWH{ 624, 432, 16, 16 };
			GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src);
			break;

		case(FRG_STAR2):
			x = (f.x >> 6) - 16;
			y = (f.y >> 6) - 16;
			src = PIXEL_LTWH{ 608, 448, 32, 32 };
			GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src);
			break;

		case(FRG_STAR3):
			x = (f.x >> 6) - 16;
			y = (f.y >> 6) - 16;
			src = PIXEL_LTWH{ 608, 448, 32, 32 };
			GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src);
			break;

		case(FRG_HEART):
			x = (f.x >> 6) - 16;
			y = (f.y >> 6) - 16;
			src = PIXEL_LTWH{ 576, 448, 32, 32 };
			GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src);
			break;

		case(FRG_FATCIRCLE):
			if(auto *gp = GrpGeom_Poly()) {
				const WINDOW_POINT center = { (f.x >> 6), (f.y >> 6) };
				gp->Lock();
				gp->SetColor({ 4, 0, 0 });
				gp->SetAlphaOne();
				GeomFatCircleA(*gp, center, ((60 - f.count) * 6), 5);
				gp->Unlock();
			}
			break;

		default:
			// ここには来ないはずだが ... //
			break;
		}
	}
}
/// ---------

/// Homing lasers
/// -------------

void _CircleA16(GRAPHICS_GEOMETRY_POLY auto& gp, int x, int y, int r, uint8_t d)
{
	VERTEX_XY	src[9 + 1];
	int			i,j;

	for(j=0,i=-64; j<=8; j++){
		src[j].x = (x + cosl(d+i, r))>>6;
		src[j].y = (y + sinl(d+i, r))>>6;
		i+=16;
	}

	src[9] = src[0];

	gp.DrawTrianglesA(TRIANGLE_PRIMITIVE::FAN, src);
}

// ホーミングレーザーを描画する //
void HLaserDraw(const HLaserData& ActiveHL)
{
	HLaserData	*hl;
	int			i,w,current;
	DegPoint	*p;
	VERTEX_XY	src[4];
	auto *gp = GrpGeom_Poly();
	auto *gf = GrpGeom_FB();
	const auto AlphaPolygon = [gp, gf](VERTEX_XY_SPAN<> p) {
		if(gp) {
			gp->DrawTrianglesA(TRIANGLE_PRIMITIVE::FAN, p);
		} else if(gf) {
			gf->DrawTriangleFan(p);
		}
	};
	// void (*AlphaCircle)(WINDOW_POINT center, WINDOW_POINT radius);

	if(gp) {
		//AlphaCircle  = GeomCircleFA;
		gp->SetColor({ 1, 2, 5 });
		gp->SetAlphaOne();
	} else if(gf) {
		// AlphaCircle  = GeomCircle;
		gf->SetColor({ 2, 2, 5 });
	}

	GrpGeom->Lock();

	for(hl = ActiveHL.Next; hl != nullptr; hl = hl->Next) {
		w = HOMINGL_WIDTH;
		current = hl->Current;
		p = &(hl->p[current]);

		// 後で最適化するのぢゃ //
		src[0].x = (p->x + cosl(p->d-64, w))>>6;
		src[0].y = (p->y + sinl(p->d-64, w))>>6;
		src[1].x = (p->x - cosl(p->d-64, w))>>6;
		src[1].y = (p->y - sinl(p->d-64, w))>>6;

		if(gp) {
			_CircleA16(*gp, p->x, p->y, w, p->d);
		} else {
			GeomCircleF({ (p->x >> 6), (p->y >> 6) }, (w >> 6));
		}

		for(i=0; i< HLASER_LEN-1; i++){
			//temp    = p;
			current = HLASER_GETPREV(current, HLASER_SECTION);
			p       = &(hl->p[current]);
			// GrpGeom->DrawLine(
			// 	(p->x >> 6), (p->y >> 6), (temp->x >> 6), (temp->y >> 6)
			// );

			src[2].x = (p->x - cosl(p->d-64, w))>>6;
			src[2].y = (p->y - sinl(p->d-64, w))>>6;
			src[3].x = (p->x + cosl(p->d-64, w))>>6;
			src[3].y = (p->y + sinl(p->d-64, w))>>6;
			AlphaPolygon(src);

			src[0] = src[3];
			src[1] = src[2];

			if(w>64*2)	w -= 64;
		}
	}

	if(gp) {
		gp->SetColor({ 3, 4, 5 });
	} else if(gf) {
		gf->SetColor({ 5, 5, 5 });
	}

	for(hl = ActiveHL.Next; hl != nullptr; hl = hl->Next) {
		w = HOMINGL_WIDTH/2;
		current = hl->Current;
		p = &(hl->p[current]);

		src[0].x = (p->x + cosl(p->d-64, w))>>6;
		src[0].y = (p->y + sinl(p->d-64, w))>>6;
		src[1].x = (p->x - cosl(p->d-64, w))>>6;
		src[1].y = (p->y - sinl(p->d-64, w))>>6;

		//AlphaCircle(p->x>>6, p->y>>6, w>>6);
		if(gp) {
			_CircleA16(*gp, p->x, p->y, w, p->d);
		} else {
			GeomCircleF({ (p->x >> 6), (p->y >> 6) }, (w >> 6));
		}

		for(i=0; i< HLASER_LEN-1; i++){
			//temp    = p;
			current = HLASER_GETPREV(current, HLASER_SECTION);
			p       = &(hl->p[current]);
			// GrpGeom->DrawLine(
			// 	(p->x >> 6), (p->y >> 6), (temp->x >> 6), (temp->y >> 6)
			// );

			src[2].x = (p->x - cosl(p->d-64, w))>>6;
			src[2].y = (p->y - sinl(p->d-64, w))>>6;
			src[3].x = (p->x + cosl(p->d-64, w))>>6;
			src[3].y = (p->y + sinl(p->d-64, w))>>6;
			AlphaPolygon(src);

			src[0] = src[3];
			src[1] = src[2];

			if(w>64)	w-=64;
			else        break;
		}
	}

	GrpGeom->Unlock();
}
/// -------------

/// Items
/// -----

// アイテムを描画する //
void ItemDraw(ITEM_DATA_CSPAN storage, std::span<const uint16_t> inds)
{
	int j, x, y;
	PIXEL_LTRB	src;

	for(const auto ind : inds) {
		auto *ip = &storage[ind];
		const uint8_t ptn = ((ip->count >> 2) & 3);
		switch(ip->type){
			case(ITEM_SCORE):
				src = PIXEL_LTWH{
					(384 + (ptn << 4)), (256 + 16), 16, 16
				};
				x = (ip->x>>6) - 8;
				y = (ip->y>>6) - 8;
				GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src);
			break;

			case(ITEM_EXTEND):
				for(j=0; j<8; j++){
					src = PIXEL_LTWH{
						(384 + (16 * 4) + (ptn << 4)), (256 + 16), 16, 16
					};
					x = (ip->x>>6) - 8 + cosl(ip->count+j*256/8, 12);
					y = (ip->y>>6) - 8 + sinl(ip->count+j*256/8, 12);
					GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src);
				}

			//	src = PIXEL_LTWH{
			//		(384 + (16 * 4) + (ptn << 4)), (256 + 16), 16, 16
			//	};
			//	x = (ip->x>>6) - 8;
			//	y = (ip->y>>6) - 8;
			//	GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src);
			break;

			case(ITEM_BOMB):
				for(j=0; j<8; j++){
					src = PIXEL_LTWH{
						(384 + (16 * 8) + (ptn << 4)), (256 + 16), 16, 16
					};
					x = (ip->x>>6) - 8 + cosl(-2*ip->count+j*256/8, 12);
					y = (ip->y>>6) - 8 + sinl(-2*ip->count+j*256/8, 12);
					GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src);
				}

			//	src = PIXEL_LTWH{
			//		(384 + (16 * 8) + (ptn << 4)), (256 + 16), 16, 16
			//	};
			//	x = (ip->x>>6) - 8;
			//	y = (ip->y>>6) - 8;
			//	GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src);
			break;

			default:
			break;
		}
	}
}
/// -----

/// Lasers
/// ------

static void SLdraw(const LASER_DATA *lp)
{
	constexpr RGB216 col = { 1, 0, 5 };
	int x,y;

	int tx,ty;


	if(lp->flag==LF_CLEAR){
		// GrpGeom->SetColor(laser_color[lp->c]);
		GrpGeom->SetColor(col);
		GrpGeom->DrawLine(lp->p[0].x, lp->p[0].y, lp->p[1].x, lp->p[1].y);
		GrpGeom->DrawLine(lp->p[3].x, lp->p[3].y, lp->p[2].x, lp->p[2].y);
		return;
	}

	x = lp->x>>6;	tx = 0;//(lp->p[0].x - x)>>2;
	y = lp->y>>6;	ty = 0;//(lp->p[0].y - y)>>2;

/* 参考
 *  レンダリングステートから考えるに、どうやらαの設定を変えてしまうと、
 *  その時点で、αが有効になってしまうドライバが存在するようである。
 *  んなもんで、ここでは GrpSetAlpha を無効にしている
 */

	// GrpGeom->SetAlpha(0, GRAPHICS_ALPHA::ONE);
	// GrpGeom->SetColor({ 4, 1, 0 });
	// GrpGeom->SetAlpha(128, GRAPHICS_ALPHA::ONE);	// 2000/09/06 に削除

	const VERTEX_XY rect[4] = { lp->p[0], lp->p[1], lp->p[2], lp->p[3] };
	if(auto *gp = GrpGeom_Poly()) {
		GeomGrdRect(*gp, rect, col.ToRGB());
	} else if(auto *gf = GrpGeom_FB()) {
		gf->SetColor({ 1, 0, 5 });
		gf->DrawTriangleFan(rect);

		gf->SetColor({ 5, 5, 5 });

		VERTEX_XY p[4];
		p[0].x = p[1].x = lp->p[0].x - lp->wx*3/4;//- wx*4/len;
		p[0].y = p[1].y = lp->p[0].y - lp->wy*3/4;//- wy*4/len;
		p[3].x = p[2].x = lp->p[3].x + lp->wx*3/4;//+ wx*4/len;
		p[3].y = p[2].y = lp->p[3].y + lp->wy*3/4;//+ wy*4/len;
		p[1].x += lp->lx;
		p[1].y += lp->ly;
		p[2].x += lp->lx;
		p[2].y += lp->ly;
		gf->DrawTriangleFan(p);
		// gf->DrawLine(lp->p[0].x, lp->p[0].y, lp->p[1].x, lp->p[1].y);
		// gf->DrawLine(lp->p[2].x, lp->p[2].y, lp->p[3].x, lp->p[3].y);
		// gf->DrawTriangleFan(lp->p, 4);
		// gf->SetColor({ 5, 5, 5 });
		// gf->DrawLine(x, y, (x + lp->lx), (y + lp->ly));
	}
	//Grp_Polygon(temp,4,RGB256(5,5,5));
}

void laser_draw(LASER_DATA_CSPAN storage, std::span<const uint16_t> inds)
{
	GrpGeom->Lock();

	for(const auto ind : inds) {
		auto *lp = &storage[ind];
		switch(lp->type){
			// ノーマルショートレーザー＆反射レーザー //
			case(LS_SHORT):case(LS_REF):
				SLdraw(lp);
			break;

			/*
			// 無限遠レーザー＆Ｙ_正方向_無限遠レーザー //
			case(LS_LONG):case(LS_LONGY):
				Ldraw(lp);
			break;
			*/
		}
	}

	/*
	for(const auto ind : inds) {
		auto *lp = &storage[ind];
		if(lp->type==LS_REF || lp->type==LS_SHORT)
			SLdraw(lp);
	}
	*/

	GrpGeom->Unlock();
}
/// ------

/// Long lasers
/// -----------

void LLaserDraw(LLASER_DATA_CSPAN llaser)
{
	int x, y;
	VERTEX_XY	p[4];
	int				wx,wy,len;

	static const RGB216 Table16Bit[16] = {
		{ 3, 0, 3 }, { 0, 2, 0 }, { 0, 0, 4 }, { 4, 2, 0 }, { 0, 0, 1 }
	};

	static const RGB216 Table8BitA[16] = {
		{ 2, 0, 2 }, { 0, 2, 0 }, { 0, 1, 3 }, { 4, 2, 0 }, { 0, 0, 1 }
	};
	static const RGB216 Table8BitB[16] = {
		{ 3, 0, 3 }, { 0, 4, 0 }, { 0, 1, 5 }, { 5, 3, 0 }, { 2, 2, 4 }
	};
	static const RGB216 Table8BitC[16] = {
		{ 5, 4, 5 }, { 5, 5, 5 }, { 4, 4, 5 }, { 5, 5, 4 }, { 4, 4, 5 }
	};


	constexpr size_t VERTEX_COUNT = 34;
	std::array<VERTEX_XY, VERTEX_COUNT> p2;

	GrpGeom->Lock();

	for(const auto& it : llaser) {
		const auto *lp = &it;
		const auto c = lp->c;
		switch(lp->flag){
			// 太さを持った状態 //
			case(LLF_OPEN):case(LLF_NORM):
			case(LLF_CLOSE):case(LLF_CLOSEL):
				x = ((lp->x)>>6)+lp->lx;
				y = ((lp->y)>>6)+lp->ly;
				wx = lp->wx;
				wy = lp->wy;
				len = isqrt(wx*wx+wy*wy);

				if(len){
					/*
					p[0].x = p[1].x = lp->p[0].x ;//- wx*4/len;
					p[0].y = p[1].y = lp->p[0].y ;//- wy*4/len;
					p[3].x = p[2].x = lp->p[3].x ;//+ wx*4/len;
					p[3].y = p[2].y = lp->p[3].y ;//+ wy*4/len;
					p[1].x += lp->infx;
					p[1].y += lp->infy;
					p[2].x += lp->infx;
					p[2].y += lp->infy;
					*/
					const VERTEX_XY rect[4] = {
						lp->p[0], lp->p[1], lp->p[2], lp->p[3]
					};
					if(auto *gp = GrpGeom_Poly()) {
						// gp->SetColor({ 3, 0, 3 });
						const RGBA col = Table16Bit[c].ToRGB().WithAlpha(0xFF);
						gp->SetAlphaOne();
						GeomGrdRectA(*gp, rect, col);

						std::array<VERTEX_RGBA, VERTEX_COUNT> vcs;
						vcs[0] = { 255, 255, 255, 0xFF };
						for(auto& vc : vcs | std::views::drop(1)) {
							vc = col;
						}

						p2[0].x = x;
						p2[0].y = y;
						p2[1].x = lp->p[0].x;
						p2[1].y = lp->p[0].y;
						p2[VERTEX_COUNT - 1].x = lp->p[3].x;
						p2[VERTEX_COUNT - 1].y = lp->p[3].y;
						for(auto n = 2; n < (VERTEX_COUNT - 1); n++) {
							p2[n].x = p2[0].x + cosl(lp->d+64+128*(n-1)/32,len);
							p2[n].y = p2[0].y + sinl(lp->d+64+128*(n-1)/32,len);
						}
						gp->DrawTrianglesA(TRIANGLE_PRIMITIVE::FAN, p2, vcs);
						break;
					} else if(auto *gf = GrpGeom_FB()) {
						// gf->SetColor({ 2, 0, 2 });
						gf->SetColor(Table8BitA[c]);
						gf->DrawTriangleFan(rect);
					}
				} else if(GrpGeom_Poly()) {
					break;
				}

				GeomCircleF({ x, y }, len); // (lp->w >> 6) + 4);

				// GrpGeom->SetColor({ 3, 0, 3 }); // lp->c;
				GrpGeom->SetColor(Table8BitB[c]);
				if(len){
					p[0].x = p[1].x = lp->p[0].x - wx/8;//+ wx*2/len;
					p[0].y = p[1].y = lp->p[0].y - wy/8;//+ wy*2/len;
					p[3].x = p[2].x = lp->p[3].x + wx/8;//- wx*2/len;
					p[3].y = p[2].y = lp->p[3].y + wy/8;//- wy*2/len;
					p[1].x += lp->infx;
					p[1].y += lp->infy;
					p[2].x += lp->infx;
					p[2].y += lp->infy;
					GrpGeom->DrawTriangleFan(p);
				}
				GeomCircleF({ x, y }, (len - len / 8)); // (lp->w >> 6) + 2);

				// GrpGeom->SetColor({ 5, 4, 5 }); // lp->c;
				GrpGeom->SetColor(Table8BitC[c]);
				if(len){
					p[0].x = p[1].x = lp->p[0].x - wx/4;//+ wx*2/len;
					p[0].y = p[1].y = lp->p[0].y - wy/4;//+ wy*2/len;
					p[3].x = p[2].x = lp->p[3].x + wx/4;//- wx*2/len;
					p[3].y = p[2].y = lp->p[3].y + wy/4;//- wy*2/len;
					p[1].x += lp->infx;
					p[1].y += lp->infy;
					p[2].x += lp->infx;
					p[2].y += lp->infy;
					GrpGeom->DrawTriangleFan(p);
				}
				GeomCircleF({ x, y }, (len - len / 4)); // (lp->w >> 6));
			break;

			// ライン状態の場合 //
			case(LLF_LINE):
				x = (lp->x)>>6;
				y = (lp->y)>>6;
				GrpGeom->SetColor({ 4, 4, 4 });
				GrpGeom->DrawLine(x, y, (x + lp->infx), (y + lp->infy));
				break;

			// 使用中で無い場合 //
			case(LLF_DISABLE):
			break;
		}
	}

	GrpGeom->Unlock();
}
/// -----------

/// VIVIT (Maid)
/// -----------

void WideBombDraw(const MAID& Viv)
{
	//PIXEL_LTRB	src = {0, 0, 382, 480};
	static PIXEL_LTRB data[6] = {
		{0,   0, 210, 240}, {210,   0, 210*2, 240}, {210*2,   0, 210*3, 240},
		{0, 240, 210, 480}, {210, 240, 210*2, 480}, {210*2, 240, 210*3, 480}
	};

	int		x,y;
	int		t;

#define BX_MIN		(X_MIN+100)
#define BY_MIN		(Y_MIN+100)

	if(Viv.weapon!=0 || Viv.bomb_time==0) return;

	x = BX_MIN;
	y = BY_MIN;

	if(Viv.bomb_time > 80){
//		x = BX_MIN + (Viv.bomb_time-30*7) * 11;	if(x < BX_MIN) x = BX_MIN;
//		y = BY_MIN + (Viv.bomb_time-30*7) * 11;	if(y < BY_MIN) y = BY_MIN;
		t = ((60*4-Viv.bomb_time)/4);
		if(t < 0) t = 0;
		if(t > 5) t = 5;
	}
	else{
//		x = BX_MIN - (80-Viv.bomb_time) * 8;
//		y = BY_MIN - (80-Viv.bomb_time) * 8;
		t = (Viv.bomb_time/4); if(t > 5) t = 5;
	}

	GrpSurface_Blit({ x, y }, SURFACE_ID::BOMBER, data[t]);
}

void LaserBombDraw(const MAID& Viv)
{
	constexpr RGBA col_channeled = RGB216{ 0, 0, 5 }.ToRGB().WithAlpha(0xFF);
	VERTEX_XY	p[4];
	int				i,w;
	int				lx,ly;
	int				wx,wy;
	const auto LaserDeg = GetLaserDeg(Viv);

	GrpGeom->Lock();
	if(LaserDeg<58){
		for(w=3;w>0;w--){
			for(i=-3;i<=3;i++){
				//64+48 - LaserDeg*3 + i*(64-LaserDeg)/2;
				const auto d = GetRightLaserDeg(LaserDeg, i);

				lx = cosl(d,850);	ly = sinl(d,850);
				wx = cosl(d+64,w);	wy = sinl(d+64,w);
				p[0].x = (Viv.opx>>6) + SBOPT_DX +  0 + wx;
				p[0].y = (Viv.opy>>6)            +  0 + wy;
				p[3].x = (Viv.opx>>6) + SBOPT_DX +  0 - wx;
				p[3].y = (Viv.opy>>6)            +  0 - wy;
				p[2].x = (Viv.opx>>6) + SBOPT_DX + lx - wx;
				p[2].y = (Viv.opy>>6)            + ly - wy;
				p[1].x = (Viv.opx>>6) + SBOPT_DX + lx + wx;
				p[1].y = (Viv.opy>>6)            + ly + wy;
				if(auto *gp = GrpGeom_Poly()) {
					gp->SetAlphaOne();
					GeomGrdRectA(*gp, p, col_channeled);
				} else if(auto *gf = GrpGeom_FB()) {
					switch(w){
					case(1):	gf->SetColor({ 4, 4, 5 });	break;
					case(2):	gf->SetColor({ 2, 2, 5 });	break;
					case(3):	gf->SetColor({ 0, 0, 5 });	break;
					}
					gf->DrawTriangleFan(p);
				}
			}
			for(i=-3;i<=3;i++){
				//64-48 + LaserDeg*3 + i*(64-LaserDeg)/2;
				const auto d = GetLeftLaserDeg(LaserDeg, i);

				lx = cosl(d,850);	ly = sinl(d,850);
				wx = cosl(d+64,w);	wy = sinl(d+64,w);
				p[0].x = (Viv.opx>>6) - SBOPT_DX +  0 + wx;
				p[0].y = (Viv.opy>>6)            +  0 + wy;
				p[3].x = (Viv.opx>>6) - SBOPT_DX +  0 - wx;
				p[3].y = (Viv.opy>>6)            +  0 - wy;
				p[2].x = (Viv.opx>>6) - SBOPT_DX + lx - wx;
				p[2].y = (Viv.opy>>6)            + ly - wy;
				p[1].x = (Viv.opx>>6) - SBOPT_DX + lx + wx;
				p[1].y = (Viv.opy>>6)            + ly + wy;
				if(auto *gp = GrpGeom_Poly()) {
					gp->SetAlphaOne();
					GeomGrdRectA(*gp, p, col_channeled);
				} else if(auto *gf = GrpGeom_FB()) {
					switch(w){
					case(1):	gf->SetColor({ 4, 4, 5 });	break;
					case(2):	gf->SetColor({ 2, 2, 5 });	break;
					case(3):	gf->SetColor({ 0, 0, 5 });	break;
					}
					gf->DrawTriangleFan(p);
				}
			}
			if(GrpGeom_Poly()) {
				break;
			}
		}
	}
	else if(LaserDeg<150){
		uint8_t c = 0;
		for(w=12-(LaserDeg-64)/8;w>0;w-=2,c++){
			for(i=-3;i<=3;i++){
				// 64+48 - 58*3 + i*(64-min(62,LaserDeg))/2;
				const auto d = GetRightLaserDeg(LaserDeg, i);

				lx = cosl(d,850);	ly = sinl(d,850);
				wx = cosl(d+64,w);	wy = sinl(d+64,w);
				p[0].x = (Viv.opx>>6) + SBOPT_DX +  0 + wx;
				p[0].y = (Viv.opy>>6)            +  0 + wy;
				p[3].x = (Viv.opx>>6) + SBOPT_DX +  0 - wx;
				p[3].y = (Viv.opy>>6)            +  0 - wy;
				p[2].x = (Viv.opx>>6) + SBOPT_DX + lx - wx;
				p[2].y = (Viv.opy>>6)            + ly - wy;
				p[1].x = (Viv.opx>>6) + SBOPT_DX + lx + wx;
				p[1].y = (Viv.opy>>6)            + ly + wy;
				if(auto *gp = GrpGeom_Poly()) {
					gp->SetAlphaOne();
					GeomGrdRectA(*gp, p, col_channeled);
				} else if(auto *gf = GrpGeom_FB()) {
					gf->SetColor({ c, c, 5u });
					gf->DrawTriangleFan(p);
				}
			}
			for(i=-3;i<=3;i++){
				//64-48 + 58*3 + i*(64-min(62,LaserDeg))/2;
				const auto d = GetLeftLaserDeg(LaserDeg, i);

				lx = cosl(d,850);	ly = sinl(d,850);
				wx = cosl(d+64,w);	wy = sinl(d+64,w);
				p[0].x = (Viv.opx>>6) - SBOPT_DX +  0 + wx;
				p[0].y = (Viv.opy>>6)            +  0 + wy;
				p[3].x = (Viv.opx>>6) - SBOPT_DX +  0 - wx;
				p[3].y = (Viv.opy>>6)            +  0 - wy;
				p[2].x = (Viv.opx>>6) - SBOPT_DX + lx - wx;
				p[2].y = (Viv.opy>>6)            + ly - wy;
				p[1].x = (Viv.opx>>6) - SBOPT_DX + lx + wx;
				p[1].y = (Viv.opy>>6)            + ly + wy;
				if(auto *gp = GrpGeom_Poly()) {
					gp->SetAlphaOne();
					GeomGrdRectA(*gp, p, col_channeled);
				} else if(auto *gf = GrpGeom_FB()) {
					gf->SetColor({ c, c, 5u });
					gf->DrawTriangleFan(p);
				}
			}
			if(GrpGeom_Poly()) {
				break;
			}
		}
	}

	GrpGeom->Unlock();
}

void MaidDraw(const MAID& Viv)
{
	static PIXEL_LTRB VivBit[4][2] = {
		{{480,128,480+24,128+24},{504,128,504+24,128+24}},	// ワイド
		{{480,152,480+24,152+24},{504,152,504+24,152+24}},	// ホーミング
		{{528,152,528+24,152+24},{552,152,552+24,152+24}},	// レーザー
		{{480,152,480+24,152+24},{504,152,504+24,152+24}},	// 仮
	};

	static uint8_t draw_flag  = 0;
	static uint8_t draw_flag2 = 0;

	const auto x  = ((Viv.x >> 6) - 16);
	const auto y  = ((Viv.y >> 6) - 24);
	const auto ox = ((Viv.opx >> 6) - 12);
	const auto oy = ((Viv.opy >> 6) - 12);
	PIXEL_LTRB	src;

	draw_flag = 1 - draw_flag;
	draw_flag2++;

	if(Viv.muteki == VIVDEAD_VAL) draw_flag = 0;

	if(Viv.muteki==0 || draw_flag){
		src = PIXEL_LTWH{ (384 + (Viv.GrpID * 32)), 128, (16 * 2), (16 * 3) };
		GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src);
	}

	if( ((Viv.exp+1)>>5) ){
		if(Viv.muteki < VIVDEAD_VAL){
			src = VivBit[Viv.weapon&3][(draw_flag2>>2)&1];
			GrpSurface_Blit({ (ox + SBOPT_DX), oy }, SURFACE_ID::SYSTEM, src);
			src = VivBit[Viv.weapon&3][(draw_flag2>>2)&1];
			GrpSurface_Blit({ (ox - SBOPT_DX), oy }, SURFACE_ID::SYSTEM, src);
		}
	}

	if(Viv.bomb_time && Viv.weapon==2)
		LaserBombDraw(Viv);
}
/// -----------

/// Player bullets (Maidtama)
/// -------------------------

// ナニな弾描画 //
void MaidTamaDraw(
	MAIDTAMA_DATA_CSPAN storage, std::span<const uint16_t> inds, const MAID& Viv
)
{
	// ここでは、さすがにTAMA.cpp 内の関数を使用するわけにはいかないので、 //
	// 独自に描画ルーチンを展開する。                                      //

	int				i,x,y;
	PIXEL_LTRB	src, ltemp;
	static PIXEL_LTRB	HomingBomb[5] = {
		{520,104,520+8 ,104+8},
		{528,104,528+16,104+16},
		{544,104,544+24,104+24},
		{568,104,568+32,104+32},
		{600,104,600+40,104+40}
	};

	for(const auto ind : inds) {
		auto *t = &storage[ind];

		x = (t->x >> 6)-8;	// -8 は座標の補正用です
		y = (t->y >> 6)-8;	// 上に同じ

		// 弾の種類により、描画指定用矩形をセットする //
		switch(t->c){
			case(TID_WIDE_MAIN):	src = PIXEL_LTWH{ (384 + ((t->d + 8) & 0xf0)), 176, 16, 16 };	break;
			case(TID_WIDE_SUB):	src = PIXEL_LTWH{ (384 + ((t->d + 8) & 0xf0)), 192, 16, 16 };	break;
			case(TID_HOMING_MAIN):	src = PIXEL_LTWH{ (384 + ((t->d + 8) & 0xf0)), 208, 16, 16 };	break;
			case(TID_HOMING_SUB):	src = PIXEL_LTWH{ (384 + ((t->d + 8) & 0xf0)), 224, 16, 16 };	break;
			case(TID_HOMING_BOMB_A):	src = PIXEL_LTWH{ (384 + ((t->d + 8) & 0xf0)), 288, 16, 16 };	break;
			case(TID_LASER_SUB):	src = PIXEL_LTWH{ (384 + ((t->d + 8) & 0xf0)), 256, 16, 16 };	break;

			case(TID_HOMING_BOMB_B):
				src = HomingBomb[(t->count/4)%5];
			break;
		}

		// 完全判定付きクリッピング //
		GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src);
	}

	// レーザーの描画 //
	if(Viv.weapon == 2  &&  Viv.lay_grp){
		ltemp = PIXEL_LTWH{ (384 + ((Viv.lay_grp - 1) << 4)), 240, 8, 16 };

		x = (Viv.opx>>6)+4 -8 + SBOPT_DX;
		y = (Viv.opy>>6)-20;
		GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, ltemp);

		x = (Viv.opx>>6)+4 -8 - SBOPT_DX;
		y = (Viv.opy>>6)-20;
		GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, ltemp);

		ltemp = PIXEL_LTWH{ (384 + 8 + ((Viv.lay_grp - 1) << 4)), 240, 8, 16 };
		for(i=(Viv.opy>>6)-36;i>-16;i-=16){
			x = (Viv.opx>>6)+4 -8 + SBOPT_DX;
			y = i;
			GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, ltemp);
		}
		for(i=(Viv.opy>>6)-36;i>-16;i-=16){
			x = (Viv.opx>>6)+4 -8 - SBOPT_DX;
			y = i;
			GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, ltemp);
		}
	}
}
/// --------------------------

/// Bullets (Tama)
/// --------------

// 弾をエフェクトとして描画？ //
void _TamaEffectDraw(const TAMA_DATA *t)
{
#define RCSET(x, y, w) {(x), (y), (x+w), (y+w)}

	static constexpr PIXEL_LTRB Data[6][5] = { // [色][パターン]
		{	// 赤 //
			RCSET(168, 344, 32), RCSET(232, 344, 28),
			RCSET(288, 344, 24), RCSET(336, 344, 20),
			RCSET(328, 416, 16),
		},
		{	// 青 //
			RCSET(168, 344+32, 32), RCSET(232, 344+28, 28),
			RCSET(288, 344+24, 24), RCSET(336, 344+20, 20),
			RCSET(328+16, 416, 16),
		},
		{	// 緑 //
			RCSET(168, 344+32*2, 32), RCSET(232, 344+28*2, 28),
			RCSET(288, 344+24*2, 24), RCSET(336, 344+20*2, 20),
			RCSET(328+16*2, 416, 16),
		},
		{	// 紫 //
			RCSET(168+32, 344, 32), RCSET(232+28, 344, 28),
			RCSET(288+24, 344, 24), RCSET(336+20, 344, 20),
			RCSET(328, 416+16, 16),
		},
		{	// 銀 //
			RCSET(168+32, 344+32, 32), RCSET(232+28, 344+28, 28),
			RCSET(288+24, 344+24, 24), RCSET(336+20, 344+20, 20),
			RCSET(328+16, 416+16, 16),
		},
		{	// 橙 //
			RCSET(168+32, 344+32*2, 32), RCSET(232+28, 344+28*2, 28),
			RCSET(288+24, 344+24*2, 24), RCSET(336+20, 344+20*2, 20),
			RCSET(328+16*2, 416+16, 16),
		},
	};
#undef RCSET

	static constexpr int Width[5] = {32/2, 28/2, 24/2, 20/2, 16/2};
	static constexpr std::span<const PIXEL_LTRB, 5> Target[16 * 3] = {
		Data[0], Data[1], Data[2], Data[3], Data[4], Data[5], Data[0], Data[0],
		Data[0], Data[0], Data[0], Data[0], Data[0], Data[0], Data[0], Data[0],

		Data[0], Data[1], Data[2], Data[3], Data[4], Data[5], Data[0], Data[0],
		Data[0], Data[0], Data[0], Data[0], Data[0], Data[0], Data[0], Data[0],

		Data[0], Data[1], Data[5], Data[3], Data[4], Data[5], Data[0], Data[0],
		Data[0], Data[0], Data[0], Data[0], Data[0], Data[0], Data[0], Data[0],
	};

	PIXEL_LTRB	temp;
	const int ptn = ((t->count / 4) % 5);
	int		x,y;

	x = (t->x >> 6) - Width[ptn];
	y = (t->y >> 6) - Width[ptn];

	// [色][パターン]
	//temp = Data[(t->c&0x0f)%6][ptn];
	if(t->c >= 16*3) temp = Target[3][ptn];
	else             temp = Target[t->c][ptn];
	GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, temp);
}

void tama_draw(
	TAMA_DATA_CSPAN storage,
	std::span<const uint16_t> inds8,
	std::span<const uint16_t> inds16
)
{
//	HRESULT		ddrval;
	PIXEL_LTRB	src;
	int			x,y;
	int			dx,dy;

	static const PIXEL_LTRB rcExtraTama[4] = {
		{128   , 384, 128+32, 384+32},
		{128+32, 384, 128+56, 384+24},
		{128+56, 384, 128+72, 384+16},
		{128+72, 384, 128+80, 384+ 8}
	};

	static constexpr uint8_t sizeExtraTama[4] = { 16, 12, 8, 4 };

	// 大型弾＆特殊弾(16*16) の描画 //
	for(const auto ind : inds16) {
		auto *t = &storage[ind];

		x = (t->x >> 6)-8;	// -8 は座標の補正用です
		y = (t->y >> 6)-8;	// 上に同じ

		switch(t->effect){
			case(TE_DELETE):
				src = PIXEL_LTWH{ (384 + ((t->count / 6) << 4)), 104, 16, 16 };
				GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src);
			continue;

			case(TE_CIRCLE1):
				_TamaEffectDraw(t);
			continue;

			// その他は知らぬ //
		}

		switch(t->c&0xf0){
			case(TAMA_LARGE):	// 大型丸弾
				src.top    = 8;
				src.left   = ((t->c&0x0f)<<4)+384;
				src.bottom = 24;
				src.right  = src.left + 16;
			break;

			case(TAMA_EXTRA): {
				const uint8_t d = (t->c & 3);
				src = rcExtraTama[d];
				x   = (t->x>>6) - sizeExtraTama[d];
				y   = (t->y>>6) - sizeExtraTama[d];
				GrpSurface_Blit({ x, y }, SURFACE_ID::ENEMY, src);
			}
			continue;

			case(TAMA_EXTRA2): {
				// The original code spelled this as
				//
				// 	d = (BYTE)(t->d+4)/8;
				//
				// which is a rather misleading way of expressing the following
				// operations:
				//
				// 1) Promote [t->d] to `int` as per C/C++'s arithmetic rules
				// 2) Add 4
				// 3) Take the least significant 8 bits to pretend that it
				//    actually was an 8-bit addition
				// 4) Divide the result by 8
				//
				// Merely removing the seemingly superfluous cast therefore
				// leads to a different result (and thus, a different sprite)
				// as the result of the integer addition is not truncated on
				// overflow. Spelling the truncation as `& 0xFF` doesn't look
				// any less superfluous. Cast::down_sign() is the best solution
				// here, as it enforces its argument to be both larger
				// (`int` > `uint8_t`) and signed.
				//
				// 256(-1) -> 32(-1) に変換
				const auto d = (Cast::down_sign<uint8_t>(t->d + 4) / 8);

				src.top    = 320 + ((t->c & 3)<<4);	// (c mod 4) * 16
				src.left   = d * 16;
				src.bottom = src.top  + 16;
				src.right  = src.left + 16;
				//x   = (t->x>>6) - 8;	// サイズは１６で固定
				//y   = (t->y>>6) - 8;	// すなわち、そのままでＯＫ！
				GrpSurface_Blit({ x, y }, SURFACE_ID::ENEMY, src);
			}
			continue;

			case(TAMA_ANGLE):
			//default:		// 角度アニメーション系
				if(t->c != 32+5){
					src.top    = 24 + ((t->c&0x0f)<<4);
					src.left   = ((t->d+8)&0xf0) + 384;
					src.bottom = src.top  + 16;
					src.right  = src.left + 16;
				}
				else{
					// Same as above.
					//
					// 修正 8 ごとで 32 分割だからズラシは 4
					// d = (Cast::down_sign<uint8_t>(t->d + 8) / 8);
					const auto d = (Cast::down_sign<uint8_t>(t->d + 4) / 8);

					dx = (d%8) * 32;
					dy = (d/8) * 32;
					src.top  = 304 + dy;
					src.left = 384 + dx;
					src.bottom = src.top  + 32;
					src.right  = src.left + 32;
					// 注意：すでに(x,y)から(8,8)が減算されているので、
					// ここでは(-16,-16)への補正のためにそれぞれ８を引く
					x-=8;	// ここであたり判定座標の
					y-=8;	// 補正を行うのだ
				}
			break;
		}

		GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src);
	}

	// 小型弾(8*8) の描画 //
	for(const auto ind : inds8) {
		auto *t = &storage[ind];

		x = (t->x >> 6)-4;	// -4 は座標の補正用です
		y = (t->y >> 6)-4;	// 上に同じ

		switch(t->effect){
			case(TE_DELETE):
				src = PIXEL_LTWH{ (384 + ((t->count / 6) << 3)), 120, 8, 8 };
				GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src);
			continue;

			case(TE_CIRCLE1):
				_TamaEffectDraw(t);
			continue;

			// その他は知らぬ //
		}

		if(t->c!=0x25){
			src.top    = 0;
			src.left   = ((t->c)<<3) + 384; //0; //(t->d+8)&0xf0;
			src.bottom = 8;
			src.right  = src.left + 8;
		}
		else{
			src.top    = 24 + ((t->c&0x0f)<<4);
			src.left   = ((t->d+8)&0xf0) + 384;
			src.bottom = src.top  + 16;
			src.right  = src.left + 16;
		}

		GrpSurface_Blit({ x, y }, SURFACE_ID::SYSTEM, src);
	}
}
/// --------------
