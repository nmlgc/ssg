/*                                                                           */
/*   EnemyExCtrl.cpp   敵用の特殊処理                                        */
/*                                                                           */
/*                                                                           */

#include "EnemyExCtrl.h"
#include "LLASER.H"
#include "LOADER.H"
#include "MAID.H"
#include "platform/graphics_backend.h"
#include "game/cast.h"
#include "game/snd.h"
#include "game/ut_math.h"

#define BIT_VIRTUAL_HP			990000		// ビットの仮想ＨＰ


SNAKYMOVE_DATA<30> SnakeData[SNAKE_MAX];
BIT_DATA		BitData;


static void BitSTDRoll(void);	// 基本的なビット回転処理
static void BitSTDRad(void);	// 基本的な半径処理



// 蛇型の敵配列の初期化 //
void SnakyInit(void)
{
	// 全ての蛇さんをダメダメにするの //
	for(auto& it : SnakeData) {
		it.bIsUse = false;
		it.Parent = nullptr;
	}
}

// 蛇型の敵をセットする //
void SnakySet(BOSS_DATA *b, int len, uint32_t TailID)
{
	ENEMY_DATA		*e;

	auto s = std::ranges::find_if(SnakeData, [](const auto& s) {
		return !s.bIsUse;
	});
	if(s == std::end(SnakeData)) {
		return;
	}

	s->bIsUse = true;
	s->Parent = b;
	s->Head   = 0;

	// Hardcoded to 30 in the original game. No way around dynamic allocation
	// if mods ever want to safely customize it.
	assert(s->Length() == len);

	// ここでは頂点バッファの初期化を行うのだ //
	// なお、ループ中断値は後で変更のこと     //
	for(auto& point : s->PointBuffer) {
		point.x = b->Edat.x;
		point.y = b->Edat.y;
		point.d = b->Edat.d;
	}

	const auto n = (4 + (TailID << 2));
	for(auto& enemy_ptr : s->EnemyPtr) {
		if(EnemyNow+1<ENEMY_MAX){
			e = &Enemy[EnemyInd[EnemyNow++]];

			InitEnemyDataX64(e,b->Edat.x,b->Edat.y,n);
			enemy_ptr = e;
		}
		else{
			enemy_ptr = nullptr; // ポインタを無効化
		}
	}
}

// 蛇型の敵の移動処理 //
void SnakyMove(void)
{
	ENEMY_DATA		*e;

	for(auto& it : SnakeData) {
		auto *s = &it;
		if(s->bIsUse == false) {
			continue;
		}

		// バッファ更新処理 //
		constexpr auto points = (s->Length() * SNAKEYMOVE_POINTS_PER_ENEMY);
		for(const auto j : std::views::iota(0u, s->Length())) {
			e = s->EnemyPtr[j];
			if(e==NULL) continue;

			const auto ptr = (
				(s->Head + points - (j * SNAKEYMOVE_POINTS_PER_ENEMY)) % points
			);

			e->x = s->PointBuffer[ptr].x;
			e->y = s->PointBuffer[ptr].y;
			e->d = s->PointBuffer[ptr].d;
		}

		s->Head = ((s->Head + 1) % points);
		s->PointBuffer[s->Head].x = s->Parent->Edat.x;
		s->PointBuffer[s->Head].y = s->Parent->Edat.y;
		s->PointBuffer[s->Head].d = s->Parent->Edat.d;
	}
}

// 蛇型の敵を殺す
void SnakyDelete(const BOSS_DATA *b)
{
	auto s = std::ranges::find_if(SnakeData, [b](const auto& s) {
		return (s.Parent == b);
	});
	if(s == std::end(SnakeData)) {
		return;
	}

	for(auto& e : s->EnemyPtr) {
		if(e == nullptr) {
			break;
		}

		// Snd_SEPlay(SOUND_ID_BOMB, e->x);
		if(e->LLaserRef) LLaserForceClose(e);	// レーザーの強制クローズ
		//PowerUp(e->hp);			// パワーアップ
		e->hp    = 0;
		e->count = 0;
		e->flag  = EF_BOMB;
		///score_add(e->score);
		//ItemSet(e->x,e->y,0);
	}

	s->bIsUse = false;
	s->Parent = NULL;
}


// ビット配列の初期化 //
void BitInit(void)
{
	int			i;

	// ここら辺の初期化には、あまり意味がない //
	BitData.x           = 0;
	BitData.y           = 0;
	BitData.BaseAngle   = 0;
	BitData.Length      = 0;
	BitData.FinalLength = 0;
	BitData.v           = 0;
	BitData.d           = 64;
	BitData.BitSpeed    = 0;
	BitData.NumBits     = 0;
//	BitData.DeltaAngle  = 0;
	BitData.LaserState  = BLASERCMD_DISABLE;
	BitData.bIsLaserEnable = false;
//	BitData.ForceCount  = 0;

	// ここから下の初期化がメインとなる //
	BitData.State  = BITCMD_DISABLE;
	BitData.Parent = NULL;

	for(i=0; i<BIT_MAX; i++){
		BitData.Bit[i].pEnemy = NULL;		// 敵データへのポインタ
		BitData.Bit[i].Angle  = 0;			// 現在の角度
		BitData.Bit[i].Force  = 0;			// その他の？力の加えられている方向
		BitData.Bit[i].BitID  = i;			// ビットの先頭からの番号
		BitData.Bit[i].BitHP  = 0;			// ビットの耐久度
	}
}


// ビットをセットする //
void BitSet(BOSS_DATA *b, uint8_t NumBits, uint32_t BitID)
{
	static const uint8_t BitHPTable[BIT_MAX] = { 1, 4, 2, 5, 3, 6 };

	int			i;

	// 他の関数と違い、不等号なので注意すべし
	// このビット構造体が有効な場合、この関数の実行はできないので
	// すぐ、リターンする
	if(BitData.State != BITCMD_DISABLE) return;

	// ビット数が不正である //
	if(NumBits == 0 || NumBits > BIT_MAX) return;

	BitData.State  = BITCMD_STDMOVE;
	BitData.Parent = b;

	BitData.x = b->Edat.x;
	BitData.y = b->Edat.y;

	BitData.Length      = 0;
	BitData.FinalLength = 64*80;
	BitData.NumBits     = NumBits;
	BitData.BitSpeed    = ((rnd()>>1)&1) ? 2: -2;
//	BitData.DeltaAngle  = (256*256)/NumBits;
	BitData.BaseAngle   = 0;//(256*256)/NumBits;//0;
	BitData.LaserState  = BLASERCMD_DISABLE;
	BitData.bIsLaserEnable = false;
//	BitData.ForceCount  = 0;

	const auto n = (4 + (BitID << 2));

	for(i=0; i<NumBits; i++){
		if(EnemyNow+1 < ENEMY_MAX){
			// 敵資源の要求 //
			auto* e = &Enemy[EnemyInd[EnemyNow++]];

			// データを初期化 //
			InitEnemyDataX64(e, BitData.x, BitData.y, n);
			e->hp    = BIT_VIRTUAL_HP;
			e->d     = i * (256 / NumBits);
			e->GR[0] = i;
			e->GR[1] = NumBits;
			parse_ECL(e);

			// この構造体と作成した敵を関連づける //
			BitData.Bit[i].pEnemy = e;		// 敵データへのポインタ
			BitData.Bit[i].Angle  = e->d;	// 現在の角度
			BitData.Bit[i].Force  = 0;		// その他の？力の加えられている方向
			BitData.Bit[i].BitID  = i;		// ビットの先頭からの番号

			BitData.Bit[i].BitHP  = 95 * BitHPTable[i];	// ビットの耐久度
		}
		else{
			BitData.Bit[i].pEnemy = NULL;
		}
	}
}


// ビットを動作させる //
void BitMove(void)
{
	int				i, j;
	ENEMY_DATA		*e;
	bool bIsDestroyed = false;

	if(BitData.NumBits == 0) return;

	switch(BitData.State){
		case BITCMD_STDMOVE:
			BitData.x = BitData.Parent->Edat.x;
			BitData.y = BitData.Parent->Edat.y;

			BitSTDRad();
			BitSTDRoll();
		break;

		case BITCMD_MOVTARGET:
			BitData.v += BitData.a;
			BitData.x += cosl(BitData.d, BitData.v);
			BitData.y += sinl(BitData.d, BitData.v);

			if(BitData.v <= -64*10) BitData.State = BITCMD_STDMOVE;

			BitSTDRad();
			BitSTDRoll();
		break;

		case BITCMD_DISABLE: default:
		return;
	}

	// レーザー放出中はダメージが無効化される //
	if(BitData.bIsLaserEnable){
		// 敵のＨＰを仮想ＨＰに回復？させる //
		// -> ダメージを蓄積させたい場合は、下のforを注釈化する事 //
		for(i=0; i<BitData.NumBits; i++){
			BitData.Bit[i].pEnemy->hp = BIT_VIRTUAL_HP;
		}
		return;
	}

	// BitData.NumBits は、削除が行われるとその数が減ることに注意 //
	for(i=0; i<BitData.NumBits; i++){
		e = BitData.Bit[i].pEnemy;
		if(e == NULL) continue;

		const uint32_t damage = (BIT_VIRTUAL_HP - e->hp);
		if(BitData.Bit[i].BitHP <= damage){
			bIsDestroyed = true;

			// ビット配列に関連づけられた敵に削除要求を送出 //
			if(e->LLaserRef) LLaserForceClose(e);
			e->hp    = 0;
			e->count = 0;			// 爆発のアニメセット用
			e->flag  = EF_BOMB;

			Snd_SEPlay(SOUND_ID_BOMB, e->x);

			for(j=i+1; j<BitData.NumBits; j++){
				BitData.Bit[j-1] = BitData.Bit[j];
				BitData.Bit[j-1].BitID--;
			}

			// 基本角となっていたビットが破壊された場合 //
			if(i == 0){
				BitData.BaseAngle += (256 / BitData.NumBits);
			}

			// ビットの総数を減らす //
			BitData.NumBits--;

			// ビット無効状態に推移する //
			if(BitData.NumBits == 0){
				BitData.State = BITCMD_DISABLE;
			}

			// 破壊されたビットの前後に力を加える                 //
			// 注意：この時点で総数はすでにデクリメントされている //
			if(BitData.NumBits){
				j = i - 1 + BitData.NumBits;
				BitData.Bit[j%BitData.NumBits].Force -= 30;
				BitData.Bit[i%BitData.NumBits].Force += 30;
			}
			//BitData.ForceCount += 60;

			// ビットの消去を行ったので、もう一度 i 番目には次のデータが格納されている //
			// したがって、次のビットの参照に移行するために、i をデクリメントする      //
			i--;
		}
		else{
			// 敵のＨＰを仮想ＨＰに回復？させる //
			e->hp = BIT_VIRTUAL_HP;

			// 真の意味で、ダメージを与えるところ //
			BitData.Bit[i].BitHP -= damage;
		}
	}

	// レジスタ更新 //
	for(i=0; i<BitData.NumBits; i++){
		e = BitData.Bit[i].pEnemy;
		if(e == NULL) continue;
		e->GR[1] = BitData.NumBits;
	}
}


// 基本的な半径処理
static void BitSTDRad(void)
{
	if(BitData.Length > BitData.FinalLength){
		BitData.Length -= 64*2;

		if(BitData.Length < BitData.FinalLength)
			BitData.Length = BitData.FinalLength;
	}
	else if(BitData.Length < BitData.FinalLength){
		BitData.Length += 64*2;

		if(BitData.Length > BitData.FinalLength)
			BitData.Length = BitData.FinalLength;
	}
}


// 基本的なビット回転処理 //
static void BitSTDRoll(void)
{
	int			i, ox, oy;
	int			n, l;

	int			dir;
	uint8_t LaserDeg;

	ENEMY_DATA	*e;
	BIT_PARAM	*bit;

	if(BitData.NumBits == 0) return;

	BitData.BaseAngle += BitData.BitSpeed;

//	if(BitData.ForceCount) BitData.ForceCount--;

	n     = BitData.NumBits;
	l     = BitData.Length;

	ox = BitData.x;
	oy = BitData.y;

	const uint8_t delta   = (256 / BitData.NumBits);
	const uint8_t ExSpeed = abs(BitData.BitSpeed / 2);

/*	if((BitData.DeltaAngle / 256) < delta){
		BitData.DeltaAngle += 64;
	}
*/

	// d       : そのビットが目標とする角度
	// delta   : ビット間の理想とする角度(収束する角度)
	// ExSpeed : そのビットの回転速度の絶対値＋１
	for(i=0; i<n; i++){
		bit = BitData.Bit + i;
		e   = bit->pEnemy;
		if(e == NULL) continue;

		// 目標とする角度を求める //
		const uint8_t d = ((BitData.BaseAngle >> 1) + (delta * bit->BitID));

		// 通常の角度収束処理 //
		dir = (Cast::sign<int8_t>(d) - Cast::sign<int8_t>(bit->Angle));

		if(     dir < -128) dir += 256;
		else if(dir >  128) dir -= 256;

		if(dir > 0) {
			if(dir > 2) dir = 2;
			//if(BitData.ForceCount)        bit->Angle += min(dir, 2);
			if(BitData.BitSpeed > 0) bit->Angle += max(dir, ExSpeed);
			else                     bit->Angle += max(dir,(ExSpeed+1));
//			if(dir > 2) bit->Angle+=ExSpeed;
//			else        bit->Angle+=(ExSpeed-1);
//			char	buf[100];
//			sprintf(buf, "dir = %d    ExSpeed = %d", dir, ExSpeed);
//			DebugOut(buf);
		}
		else if(dir < 0){
			if(dir < -2) dir = -2;
//			if(BitData.ForceCount)        bit->Angle -= min(-dir, 2);
			if(BitData.BitSpeed < 0) bit->Angle -= max(-dir, ExSpeed);
			else                     bit->Angle -= max(-dir,(ExSpeed+1));
//			if(dir < -2) bit->Angle-=ExSpeed;
//			else         bit->Angle-=(ExSpeed-1);
//			char	buf[100];
//			sprintf(buf, "dir = %d    ExSpeed = %d", dir, ExSpeed);
//			DebugOut(buf);
		}

		// 力による影響を反映する //
		if(bit->Force > 0){
			bit->Force--;
			if(BitData.BitSpeed > 0) bit->Angle++;
			else                     bit->Angle+=(ExSpeed+1);
			// Sleep(100);
		}
		else if(bit->Force < 0){
			bit->Force++;
			if(BitData.BitSpeed < 0) bit->Angle--;
			else                     bit->Angle-=(ExSpeed+1);
			// Sleep(100);
		}

		e->d = bit->Angle;
		e->x = ox + cosl(e->d, l);
		e->y = oy + sinl(e->d, l);

		// レーザーコマンドの反映 //
		switch(BitData.LaserState){
			case(BLASERCMD_TYPE_A):		// 一方向・角度固定レーザーを放射
			case(BLASERCMD_TYPE_B):		// 両方向角度固定レーザーを放射
			break;

			case(BLASERCMD_TYPE_C):		// 角度同期ｎ芒星レーザー
				if(BitData.NumBits == 0) break;
				LaserDeg = 64 + 256 / BitData.NumBits;
				LLaserDegA(e, e->d + LaserDeg, 0);
				LLaserDegA(e, e->d - LaserDeg, 1);
			break;
		}
	}
}

// ビットを消滅させる //
void BitDelete(void)
{
	int				i;
	ENEMY_DATA		*e;

	if(BitData.State == BITCMD_DISABLE) return;

	// 各ビットを消滅させる //
	for(i=0; i<BitData.NumBits; i++){
		e = BitData.Bit[i].pEnemy;
		if(e == NULL) continue;

		if(e->LLaserRef) LLaserForceClose(e);
		e->hp    = 0;
		e->count = 0;
		e->flag  = EF_BOMB;

		Snd_SEPlay(SOUND_ID_BOMB, e->x);
	}

	// 後は、この関数に任せる //
	BitInit();
}


// ビット間のラインを描画する //
void BitLineDraw(void)
{
	int				i, j, n;
	int				x1, x2, y1, y2;
	ENEMY_DATA		*RefTable[BIT_MAX*2];

	if(BitData.State == BITCMD_DISABLE) return;

	n = BitData.NumBits;
	if(n == 0) return;

	for(i=0, j=-1; i<n; i++){
		while(BitData.Bit[++j].pEnemy == NULL);

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


// 攻撃パターンをセットor変更 //
void BitSelectAttack(uint32_t BitID)
{
	int			i;

	const auto n = (4 + (BitID << 2));

	for(i=0; i<BitData.NumBits; i++){
		EnemyECL_LongJump(BitData.Bit[i].pEnemy, n);
	}
}

// レーザー系命令を発行 //
void BitLaserCommand(uint8_t Command)
{
	int				i;
	ENEMY_DATA		*e;
	uint8_t delta;

	LLaserCmd.dx = 0;
	LLaserCmd.dy = 0;
	LLaserCmd.v  = 64;
	LLaserCmd.w  = 64*8;

	BitData.bIsLaserEnable = true;

	for(i=0; i<BitData.NumBits; i++){
		e = BitData.Bit[i].pEnemy;
		if(e == NULL) continue;

		LLaserCmd.e    = e;
		LLaserCmd.d    = e->d;

		switch(Command){
			case(BLASERCMD_TYPE_A):		// 一方向・角度固定レーザーを放射
				LLaserCmd.type = LLS_LONG;
				LLaserCmd.c    = 2;
				if(LLaserSet(e->LLaserRef)) e->LLaserRef++;
			break;

			case(BLASERCMD_TYPE_B):		// 両方向角度固定レーザーを放射
				LLaserCmd.d += 64;
				LLaserCmd.type = LLS_LONG;
				LLaserCmd.c = 1;
				if(LLaserSet(e->LLaserRef)) e->LLaserRef++;

				LLaserCmd.d += 128;
				if(LLaserSet(e->LLaserRef)) e->LLaserRef++;
			break;

			case(BLASERCMD_TYPE_C):		// 角度同期ｎ芒星レーザー
				LLaserCmd.type = LLS_LONG;
				LLaserCmd.c = 0;

				delta = 64 + 256 / BitData.NumBits;

				LLaserCmd.d = e->d + delta;
				if(LLaserSet(e->LLaserRef)) e->LLaserRef++;
				LLaserCmd.d = e->d - delta;
				if(LLaserSet(e->LLaserRef)) e->LLaserRef++;
			break;

			case(BLASERCMD_OPEN):
				LLaserOpen(e, ECLCST_LLASERALL);
			continue;

			case(BLASERCMD_CLOSE):
				LLaserClose(e, ECLCST_LLASERALL);
				e->LLaserRef = 0;
				BitData.bIsLaserEnable = false;
			continue;

			case(BLASERCMD_CLOSEL):
				LLaserLine(e, ECLCST_LLASERALL);
			continue;
		}

		BitData.LaserState = Command;
	}
}


// ビット命令を送信 //
void BitSendCommand(uint8_t Command, int Param)
{
	switch(Command){
		case(BITCMD_CHGSPD):		// 回転速度を変更する
			// 同じ方向で、速度を変更する
			if(Param > 0){
				if(BitData.BitSpeed > 0) BitData.BitSpeed = Param;
				else                     BitData.BitSpeed = -Param;
			}
			// 回転方向を反転し、速度を変更する
			else{
				if(BitData.BitSpeed > 0) BitData.BitSpeed = Param;
				else                     BitData.BitSpeed = -Param;
			}
		break;

		case(BITCMD_CHGRADIUS):		// 半径を変更する
			BitData.FinalLength = Param;
		break;

		case(BITCMD_MOVTARGET):		// 目標(びびっと)に向けてブーメラン移動
			BitData.v = 64*10;
			BitData.a = -8;
			BitData.d = atan8(Viv.x-BitData.x, Viv.y-BitData.y);
			BitData.State = BITCMD_MOVTARGET;
		break;

		default:
		return;
	}
}


// 現在のビット数を取得する //
int BitGetNum(void)
{
	if(BitData.State == BITCMD_DISABLE) return 0;
	return BitData.NumBits;
}
