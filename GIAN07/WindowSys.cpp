/*                                                                           */
/*   WINDOWSYS.cpp   コマンドウィンドウ処理                                  */
/*                                                                           */
/*                                                                           */

#include "WindowSys.h"
#include "LOADER.H"
#include "FONTUTY.H"
#include "DirectXUTYs/DD_UTY.H"
#include "game/snd.h"
#include "game/ut_math.h"


// Constants
// ---------

constexpr auto CWIN_FONT = GIAN_FONT_ID::SMALL;

constexpr PIXEL_COORD CWIN_ITEM_LEFT = 8;
constexpr PIXEL_COORD CWIN_ITEM_H = 16;
constexpr PIXEL_COORD CWIN_MAX_H = ((WINITEM_MAX + 1) * CWIN_ITEM_H);

constexpr PIXEL_COORD FACE_W = 96;
constexpr PIXEL_COORD FACE_H = 96;

constexpr PIXEL_POINT MSG_TEXT_TOPLEFT = { FACE_W, 8 };
// ---------

///// [構造体] /////

// メッセージウィンドウ管理用構造体 //
typedef struct tagMSG_WINDOW{
	WINDOW_LTRB	MaxSize;	// ウィンドウの最終的な大きさ
	WINDOW_LTRB	NowSize;	// ウィンドウの現在のサイズ

	GIAN_FONT_ID	FontID;	// 使用するフォント
	uint8_t	FontDy;	// フォントのＹ増量値
	uint8_t	State;	// 状態
	uint8_t	MaxLine;	// 最大表示可能行数
	uint8_t	Line;	// 次に挿入する行

	uint8_t	FaceID;	// 使用する顔番号
	uint8_t	NextFace;	// 次に表示する顔番号
	uint8_t	FaceState;	// 顔の状態
	uint8_t	FaceTime;	// 顔表示用カウンタ

	Narrow::string_view	Msg[MSG_HEIGHT];	// 表示するメッセージへのポインタ

	// Contains all text from [Msg], concatenated with '\n'.
	Narrow::string	Text;

	std::optional<TEXTRENDER_RECT_ID>	TRR;

	void MsgBlank() {
		Line = 0;
		for(auto msg : Msg) {
			msg = {};
		}
		Text.clear();
	}

} MSG_WINDOW;



///// [非公開関数] /////

static WINDOW_INFO *CWinSearchActive(WINDOW_SYSTEM *ws);	// アクティブなウィンドウを探す
static void CWinKeyEvent(WINDOW_SYSTEM *ws);				// キーボード入力を処理する

static void DrawWindowFrame(int x,int y,int w,int h);		// ウィンドウ枠を描画する
static void GrpBoxA2(int x1,int y1,int x2,int y2);			// 平行四辺形ＢＯＸ描画



///// [グローバル変数] /////

MSG_WINDOW		MsgWindow;		// メッセージウィンドウ



uint8_t WINDOW_INFO::MaxItems() const
{
	uint8_t ret = NumItems;
	for(auto i = 0; i < NumItems; i++) {
		ret = (std::max)(ret, ItemPtr[i]->MaxItems());
	}
	return ret;
}

void WINDOW_INFO::SetActive(bool active)
{
	State = (active ? STATE::REGULAR : STATE::DISABLED);
}

void WINDOW_SYSTEM::Init(PIXEL_COORD w)
{
	W = w;

	// Don't forget the header.
	const auto max_items = (1 + Parent.MaxItems());

	for(auto i = 0; i < max_items; i++) {
		TRRs[i] = TextObj.Register({ W, CWIN_ITEM_H });
	}
}

void WINDOW_SYSTEM::Open(WINDOW_POINT topleft, int select)
{
	x = topleft.x;
	y = topleft.y;

	Count       = 0;
	Select[0]   = select;
	SelectDepth = 0;
	State       = CWIN_INIT;

	OldKey   = Key_Data;
	KeyCount = CWIN_KEYWAIT;

	FirstWait = true;
}

void WINDOW_SYSTEM::OpenCentered(PIXEL_COORD w, int select)
{
	// Shifting it down by 9 pixels avoids the clash with the background image
	// gradient.
	WINDOW_POINT topleft = {
		(320 - (w / 2)),
		(73 + (CWIN_MAX_H / 2) - (((Parent.NumItems + 1) * CWIN_ITEM_H) / 2))
	};
	return Open(topleft, select);
}

// コマンドウィンドウを１フレーム動作させる //
void CWinMove(WINDOW_SYSTEM *ws)
{
	switch(ws->State){
		case(CWIN_DEAD):	// 使用されていない
		return;

		case(CWIN_FREE):	// 入力待ち状態
			CWinKeyEvent(ws);
			ws->Count = 0;
		return;

		case(CWIN_OPEN):	// 項目移動中(進む)
		break;

		case(CWIN_CLOSE):	// 項目移動中(戻る)
		break;

		case(CWIN_NEXT):	// 次のウィンドウに移行中
		break;

		case(CWIN_BEFORE):	// 前のウィンドウに移行中
		break;

		case(CWIN_INIT):	// 初期化処理中
			ws->State = CWIN_FREE;
		break;
	}

	ws->Count++;
}

// コマンドウィンドウの描画 //
void CWinDraw(WINDOW_SYSTEM *ws)
{
	struct COLOR_PAIR {
		RGBA shadow;
		RGBA text;
	};

	static constexpr ENUMARRAY<COLOR_PAIR, WINDOW_INFO::STATE> COL = {{
		COLOR_PAIR{ { 128, 128, 128 }, { 255, 255, 255 } }, // Regular
		COLOR_PAIR{ { 128, 128, 128 }, { 255, 255,  70 } }, // Highlight
		COLOR_PAIR{ {  96,  96,  96 }, { 192, 192, 192 } }, // Disabled
	}};
	WINDOW_INFO		*p;
	int				i;
	HDC				hdc;
	BYTE			alpha;
	WINDOW_COORD	top = ws->y;

	// アクティブな項目を検索する //
	p = CWinSearchActive(ws);

	// 半透明ＢＯＸの描画 //
	alpha = (DxObj.PixelFormat.IsPalettized()) ? 64+32 : 128;

	GrpLock();
	GrpSetAlpha(alpha,ALPHA_NORM);

	GrpSetColor(0,0,0);
	GrpBoxA(ws->x, top, (ws->x + ws->W), (top + CWIN_ITEM_H));
	top += CWIN_ITEM_H;

	GrpSetColor(0,0,2);
	for(i=0;i<p->NumItems;i++){
		if(i==ws->Select[ws->SelectDepth]){
			GrpSetAlpha(128,ALPHA_NORM);
			GrpSetColor(5,0,0);
		}
		GrpBoxA(ws->x, top, (ws->x + ws->W), (top + CWIN_ITEM_H));
		top += CWIN_ITEM_H;
		if(i==ws->Select[ws->SelectDepth]){
			GrpSetAlpha(alpha,ALPHA_NORM);
			GrpSetColor(0,0,2);
		}
	}
	GrpUnlock();

	// 文字列の描画 //
	WINDOW_POINT topleft = { ws->x, ws->y };
	const auto trr = ws->TRRs[0];
	const Narrow::string_view str = p->Title;
	TextObj.Render(topleft, trr, str, [=](GIAN_TEXTRENDER_SESSION auto& s) {
		const auto& col = COL[WINDOW_INFO::STATE::REGULAR];
		s.SetFont(CWIN_FONT);
		s.Put({ 1, 0 }, str, col.shadow);
		s.Put({ 0, 0 }, str, col.text);
	});
	topleft.y += (CWIN_ITEM_H + 1); // ???

	for(i = 0; i < p->NumItems; i++) {
		const auto trr = ws->TRRs[1 + i];
		auto* item = p->ItemPtr[i];
		const Narrow::string_view str = item->Title;
		const Narrow::string_view c = ((item->State == item->StatePrev)
			? str
			: ""
		);
		TextObj.Render(topleft, trr, c, [=](GIAN_TEXTRENDER_SESSION auto& s) {
			const auto& col = COL[item->State];
			s.SetFont(CWIN_FONT);
			s.Put({ (CWIN_ITEM_LEFT + 1), 0 }, str, col.shadow);
			s.Put({ (CWIN_ITEM_LEFT + 0), 0 }, str, col.text);
		});
		item->StatePrev = item->State;
		topleft.y += CWIN_ITEM_H;
	}
}

// コマンド [Exit] のデフォルト処理関数 //
bool CWinExitFn(INPUT_BITS key)
{
	switch(key){
		case(KEY_RETURN):case(KEY_TAMA):case(KEY_BOMB):case(KEY_ESC):
		return FALSE;

		default:
		return TRUE;
	}
}

PIXEL_SIZE CWinTextExtent(Narrow::string_view str)
{
	return TextObj.TextExtent(GIAN_FONT_ID::SMALL, str);
}

PIXEL_SIZE CWinItemExtent(Narrow::string_view str)
{
	auto ret = CWinTextExtent(str);
	ret.w += CWIN_ITEM_LEFT;
	ret.h = CWIN_ITEM_H;
	return ret;
}

void MWinInit(const WINDOW_LTRB& rc)
{
	MsgWindow.MaxSize = rc;
	MsgWindow.TRR = TextObj.Register(rc.Size() - MSG_TEXT_TOPLEFT);
}

void MWinOpen(void)
{
	if(MsgWindow.State != MWIN_DEAD) return;

	// 状態および、最終値のセット //
	MsgWindow.State     = MWIN_OPEN;
	MsgWindow.FaceState = MFACE_NONE;			// 何も表示しない
	MsgWindow.FaceID    = 0;
	MsgWindow.FaceTime  = 0;

	// 矩形の初期値をセットする //
	auto y_mid = ((MsgWindow.MaxSize.bottom + MsgWindow.MaxSize.top) / 2);
	MsgWindow.NowSize.left   = MsgWindow.MaxSize.left;
	MsgWindow.NowSize.right  = MsgWindow.MaxSize.right;
	MsgWindow.NowSize.top    = y_mid - 4;
	MsgWindow.NowSize.bottom = y_mid + 4;

	MWinCmd(MWCMD_NORMALFONT);					// ノーマルフォント

	// ウィンドウ内に表示するものの初期化 //
	MsgWindow.MsgBlank();
}

// メッセージウィンドウをクローズする //
void MWinClose(void)
{
//	if(MsgWindow.State != MWIN_FREE) return;

	MsgWindow.FaceState = MFACE_CLOSE;
	MsgWindow.State     = MWIN_CLOSE;
}

// メッセージウィンドウを強制クローズする //
void MWinForceClose(void)
{
	MsgWindow.FaceState = MFACE_NONE;
	MsgWindow.State     = MWIN_DEAD;
}

// メッセージウィンドウを動作させる //
void MWinMove(void)
{
	switch(MsgWindow.FaceState){
		case(MFACE_OPEN):		// 顔を表示しようとしている
			MsgWindow.FaceTime+=16;
			if(MsgWindow.FaceTime==0) MsgWindow.FaceState = MFACE_WAIT;
		break;

		case(MFACE_NEXT):
			MsgWindow.FaceTime+=16;
			if(MsgWindow.FaceTime==0){
				MsgWindow.FaceState = MFACE_OPEN;
				MsgWindow.FaceID    = MsgWindow.NextFace;
				GrpSetPalette(FaceData[MsgWindow.FaceID/FACE_NUMX].pal);
			}
		break;

		case(MFACE_CLOSE):		// 顔を消そうとしている
			MsgWindow.FaceTime+=16;
			if(MsgWindow.FaceTime==0) MsgWindow.FaceState = MFACE_NONE;
		break;

		default:
		break;
	}

	switch(MsgWindow.State){
		case(MWIN_OPEN):
			MsgWindow.NowSize.top    -= 2;
			MsgWindow.NowSize.bottom += 2;

			// 完全にオープンできた場合 //
			if(MsgWindow.NowSize.top <= MsgWindow.MaxSize.top){
				MsgWindow.NowSize = MsgWindow.MaxSize;
				MsgWindow.State   = MWIN_FREE;
			}
		break;

		case(MWIN_CLOSE):
			MsgWindow.NowSize.top    += 3;
			MsgWindow.NowSize.bottom -= 3;
			MsgWindow.NowSize.right  += 6;
			MsgWindow.NowSize.left   -= 6;

			// 完全にクローズできた場合 //
			if(MsgWindow.NowSize.top >= MsgWindow.NowSize.bottom){
				MsgWindow.State   = MWIN_DEAD;
			}
		break;

		case(MWIN_DEAD):	case(MWIN_FREE):
		break;
	}
}

// メッセージウィンドウを描画する(上に同じ) //
void MWinDraw(void)
{
	BYTE	alpha;
	PIXEL_LTRB	src;

	int		x = MsgWindow.NowSize.left;		// ウィンドウ左上Ｘ
	int		y = MsgWindow.NowSize.top;		// ウィンドウ左上Ｙ
	int		w = (MsgWindow.NowSize.right  - x);		// ウィンドウ幅
	int		h = (MsgWindow.NowSize.bottom - y);		// ウィンドウ高さ
	int		len,time,oy;

	// メッセージウィンドウが死んでいたら何もしない //
	if(MsgWindow.State == MWIN_DEAD) return;

	// 半透明部の描画 //
	GrpLock();
	alpha = (DxObj.PixelFormat.IsPalettized()) ? 64+32 : 110;
	GrpSetAlpha(alpha,ALPHA_NORM);
	GrpSetColor(0,0,3);//0,0,3
	GrpBoxA(x+4,y+4,x+w-4,y+h-4);
	GrpUnlock();

	// 文字列を表示するのはウィンドウが[FREE]である場合だけ        //
	// -> こうしないと文字列用 Surface を作成することになるので... //
	if((MsgWindow.State == MWIN_FREE) && MsgWindow.TRR) {
		const auto topleft = (WINDOW_POINT{ x, y } + MSG_TEXT_TOPLEFT);
		const auto trr = MsgWindow.TRR.value();
		const auto& text = MsgWindow.Text;
		TextObj.Render(topleft, trr, text, [](GIAN_TEXTRENDER_SESSION auto& s) {
			// セットされたフォントで描画
			s.SetFont(MsgWindow.FontID);
			for(auto i = 0; i < MsgWindow.Line; i++) {
				const auto line = MsgWindow.Msg[i];

				// 一応安全対策
				if(line.empty()) {
					continue;
				}
				const PIXEL_COORD top = (i * MsgWindow.FontDy);

				// 灰色で１どっとずらして描画
				s.Put({ 1, top }, line, RGBA{ 128, 128, 128 });
				// 白で表示すべき位置に表示
				s.Put({ 0, top }, line, RGBA{ 255, 255, 255 });
			}
		});
	}

	DrawWindowFrame(x,y,w,h);

	// お顔をかきましょう(表示を要請されている場合にだけ) //
	auto& GrFace = GrFaces[MsgWindow.FaceID / FACE_NUMX];
	switch(MsgWindow.FaceState){
		case(MFACE_WAIT):
			oy = MsgWindow.MaxSize.bottom - 100;
			src = PIXEL_LTWH{
				((MsgWindow.FaceID % FACE_NUMX) * FACE_W), 0, FACE_W, FACE_H
			};
			GrpBlt(&src,x+2,oy,GrFace);
		break;

		case(MFACE_OPEN):
			time = MsgWindow.FaceTime>>2;
			oy = MsgWindow.MaxSize.bottom - 100;
			for(auto i = 0; i < FACE_H; i++){
				len = cosl(time+i*153,(64-time)/2);
				//len = cosl(time+i*4,64-time);
				src = PIXEL_LTWH{
					((MsgWindow.FaceID % FACE_NUMX) * FACE_W), i, FACE_W, 1
				};
				GrpBlt(&src,x+len+2,oy+i,GrFace);
				//if(i&1)	GrpBlt(&src,x+len+2,oy+i,GrFace);
				//else	GrpBlt(&src,x-len+2,oy+i,GrFace);
			}
		break;

		case(MFACE_NEXT):
			time = (255-MsgWindow.FaceTime)>>2;
			oy = MsgWindow.MaxSize.bottom - 100;
			for(auto i = 0; i < FACE_H; i++){
				len = cosl(time+i*153,(64-time)/2);
				//len = cosl(time+i*4,64-time);
				src = PIXEL_LTWH{
					((MsgWindow.FaceID % FACE_NUMX) * FACE_W), i, FACE_W, 1
				};
				GrpBlt(&src,x+len+2,oy+i,GrFace);
				//if(i&1)	GrpBlt(&src,x+len+2,oy+i,GrFace);
				//else	GrpBlt(&src,x-len+2,oy+i,GrFace);
			}
		break;

		case(MFACE_CLOSE):
			time = MsgWindow.FaceTime>>1;
			oy = MsgWindow.MaxSize.bottom - 100;
			for(auto i = 0; i < FACE_H; i++){
				len = cosl(time+i*4,time);
				src = PIXEL_LTWH{
					((MsgWindow.FaceID % FACE_NUMX) * FACE_W), i, FACE_W, 1
				};
				if(i&1)	GrpBlt(&src,x-len+2,oy+i,GrFace);
				else	GrpBlt(&src,x+len+2,oy+i,GrFace);
			}
		break;
	}
}

void MWinMsg(Narrow::string_view s)
{
	int			Line,i;

	Line = MsgWindow.Line;

	if(Line==MsgWindow.MaxLine){
		// すでに表示最大行数を超えていた場合 //
		for(i=1;i<MsgWindow.MaxLine-1;i++) MsgWindow.Msg[i] = MsgWindow.Msg[i+1];
		MsgWindow.Msg[Line-1] = s;
	} else {
		// ポインタセット＆行数更新 //
		MsgWindow.Msg[Line] = s;
		MsgWindow.Line = Line+1;
	}

	MsgWindow.Text.clear();
	for(const auto s : MsgWindow.Msg) {
		MsgWindow.Text += s;
		MsgWindow.Text += '\n';
	}
}

// 顔をセットする //
void MWinFace(uint8_t faceID)
{
	if(MsgWindow.State==MWIN_DEAD) return;		// 表示不可
	if(faceID/FACE_NUMX>=FACE_MAX) return;		// あり得ない数字

	if(MsgWindow.FaceState==MFACE_NONE){
		MsgWindow.FaceState = MFACE_OPEN;
		MsgWindow.FaceID = faceID;
		GrpSetPalette(FaceData[faceID/FACE_NUMX].pal);
	}
	else{
		MsgWindow.FaceState = MFACE_NEXT;
		MsgWindow.NextFace  = faceID;
	}

	MsgWindow.FaceTime  = 0;
}

// コマンドを送る //
void MWinCmd(uint8_t cmd)
{
	int		temp,i;
	int		Ysize = 0;

	switch(cmd){
		case(MWCMD_LARGEFONT):		// ラージフォントを使用する
			Ysize +=  8;
		case(MWCMD_NORMALFONT):		// ノーマルフォントを使用する
			Ysize +=  2;
		case(MWCMD_SMALLFONT):		// スモールフォントを使用する
			Ysize += 14;
			temp = MsgWindow.MaxSize.bottom - MsgWindow.MaxSize.top - 16;
			MsgWindow.MaxLine = temp / Ysize;							// 表示可能最大行数
			MsgWindow.FontDy  =(temp % Ysize)/(temp/Ysize)+Ysize + 1;	// Ｙ増量
			MsgWindow.FontID  = GIAN_FONT_ID(cmd);	// 使用フォント

		case(MWCMD_NEWPAGE):		// 改ページする
			// 文字列無効化, 最初の行へ
			MsgWindow.MsgBlank();
		break;

		default:		// ここに来たらバグね...
		break;
	}
}

// ウィンドウ枠を描画する //
static void DrawWindowFrame(int x,int y,int w,int h)
{
	PIXEL_LTRB	src;

	w = w >> 1;
	h = h >> 1;

	// 左上 //
	src = { 0, 0, w, h };
	GrpBlt(&src,x,y,GrTama);

	// 右上 //
	src = { (384 - w), 0, 384, h };
	GrpBlt(&src,x+w,y,GrTama);

	// 左下 //
	src = { 0, (80 - h), w, 80 };
	GrpBlt(&src,x,y+h,GrTama);

	// 右下 //
	src = { (384 - w), (80 - h), 384, 80 };
	GrpBlt(&src,x+w,y+h,GrTama);
}

// ヘルプ文字列を送る //
void MWinHelp(WINDOW_SYSTEM *ws)
{
	WINDOW_INFO		*p;

	// アクティブなウィンドウを検索し、メッセージ領域をクリアする //
	p = CWinSearchActive(ws);
	MsgWindow.MsgBlank();

	// 一列だけ文字列を割り当てる //
	MWinMsg(p->ItemPtr[ws->Select[ws->SelectDepth]]->Help);
}

// アクティブなウィンドウを探す //
static WINDOW_INFO *CWinSearchActive(WINDOW_SYSTEM *ws)
{
	WINDOW_INFO		*p;
	int				i;

	// 現在アクティブな項目を探す //
	p = &(ws->Parent);
	for(i=0;i<ws->SelectDepth;i++){
		p = p->ItemPtr[ws->Select[i]];
	}

	return p;
}

// キーボード入力を処理する //
static void CWinKeyEvent(WINDOW_SYSTEM *ws)
{
	using STATE = WINDOW_INFO::STATE;

	if(ws->FirstWait){
		if(Key_Data) return;
		else         ws->FirstWait = FALSE;
	}

	// 操作性が悪かった点を修正 //
	if(Key_Data==0 && ws->KeyCount){
		ws->OldKey   = 0;
		ws->KeyCount = 0;
		return;
	}

	// アクティブなウィンドウを検索する //
	auto* p = CWinSearchActive(ws);
	auto Depth = ws->SelectDepth;

	// アクティブな項目をセットする //
	auto* p2 = p->ItemPtr[ws->Select[Depth]];

	assert(p2->State != STATE::DISABLED);

	// キーボードの過剰なリピート防止 //
	switch(ws->OldKey){
		// 一定間隔でリピートを許可するキー //
		case(KEY_UP):case(KEY_DOWN):case(KEY_LEFT):case(KEY_RIGHT):
			if(ws->KeyCount){
				ws->KeyCount--;
				if(ws->KeyCount==0) ws->OldKey=0;
			}
			else ws->KeyCount = CWIN_KEYWAIT;
		return;

		// いかなる場合もリピートを許可しないキー //
		case(KEY_TAMA):case(KEY_BOMB):case(KEY_RETURN):case(KEY_ESC):
			if(Key_Data == ws->OldKey) return;

		default:
			ws->KeyCount = 0;
		break;
	}

	ws->OldKey = Key_Data;

	// 一部のキーボード入力を処理する(KEY_UP/KEY_DOWN) //
	switch(Key_Data){
		case(KEY_UP): // 一つ上の項目へ
			do {
				ws->Select[Depth] = (
					(ws->Select[Depth] + p->NumItems - 1) % p->NumItems
				);
			} while(p->ItemPtr[ws->Select[Depth]]->State == STATE::DISABLED);
			Snd_SEPlay(SOUND_ID_SELECT);
			break;

		case(KEY_DOWN): // 一つ下の項目へ
			do {
				ws->Select[Depth] = ((ws->Select[Depth] + 1) % p->NumItems);
			} while(p->ItemPtr[ws->Select[Depth]]->State == STATE::DISABLED);
			Snd_SEPlay(SOUND_ID_SELECT);
			break;

		case(KEY_ESC):case(KEY_BOMB):
			Snd_SEPlay(SOUND_ID_CANCEL);
		break;

		case(KEY_TAMA):case(KEY_RETURN):case(KEY_LEFT):case(KEY_RIGHT):
			Snd_SEPlay(SOUND_ID_SELECT);
		break;
	}

	if(p2->CallBackFn != NULL){
		// コールバック動作時の処理 //
		if(p2->CallBackFn(Key_Data)==FALSE){
			if(Depth==0){
				if(Key_Data!=KEY_ESC && Key_Data!=KEY_BOMB){
					// 後で (CWIN_CLOSE) に変更すること//
					ws->State  = CWIN_DEAD;
					ws->OldKey = 0;			// ここは結構重要
				}
			}
			else{
				if(ws->SelectDepth != 0){
					ws->SelectDepth--;
				}
			}
		}
	}
	else{
		// デフォルトのキーボード動作 //
		switch(Key_Data){
			case(KEY_TAMA):case(KEY_RETURN):	// 決定・選択
				if(p2->NumItems!=0){
					ws->Select[Depth+1] = 0;
					ws->SelectDepth++;
				}
			break;

			case(KEY_BOMB):case(KEY_ESC):		// キャンセル
				if(ws->SelectDepth != 0){
					ws->SelectDepth--;
				}
			break;
		}
	}
}

// 平行四辺形ＢＯＸ描画 //
static void GrpBoxA2(int x1,int y1,int x2,int y2)
{
	int i;

	for(i=0;i<=y2-y1;i++)
		GrpBoxA(x1,y1+i,x2+i*2,y1+i+1);
}
