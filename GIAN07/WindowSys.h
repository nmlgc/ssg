/*                                                                           */
/*   WINDOWSYS.h   コマンドウィンドウ処理                                    */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_WINDOWSYS_H
#define PBGWIN_WINDOWSYS_H		"WINDOWSYS : Version 0.24 : Update 2000/02/28"

#include "FONTUTY.H"
#include "game/coords.h"
#include "game/input.h"



///// [更新履歴] /////
/*
 * 2000/07/26 : 操作性を改善した
 *
 * 2000/02/28 : メッセージウィンドウの半透明部を高速化
 * 2000/02/10 : PWINDOW->WINDOWSYS
 *            : WINDOWSYS において、コマンドウィンドウは、スタックで選択状態を保持する。
 *            : ウィンドウを動かしたり枠を付けたりするのはあとでね...
 *
 * 2000/01/31 : 開発はじめ
 *
 */



// 最大数に関する定数 //
#define WINITEM_MAX		20			// 項目の最大数
#define WINDOW_DEPTH	10			// ウィンドウの深さ
#define MSG_HEIGHT		5			// メッセージウィンドウの高さ

// コマンドウィンドウの状態 //
#define CWIN_DEAD		0x00		// 使用されていない
#define CWIN_FREE		0x01		// 入力待ち状態
#define CWIN_OPEN		0x02		// 項目移動中(進む)
#define CWIN_CLOSE		0x03		// 項目移動中(戻る)
#define CWIN_NEXT		0x04		// 次のウィンドウに移行中
#define CWIN_BEFORE		0x05		// 前のウィンドウに移行中
#define CWIN_INIT		0xff		// 初期化処理中

// メッセージウィンドウ・コマンド //
#define MWCMD_SMALLFONT		0x00		// スモールフォントを使用する
#define MWCMD_NORMALFONT	0x01		// ノーマルフォントを使用する
#define MWCMD_LARGEFONT		0x02		// ラージフォントを使用する
#define MWCMD_NEWPAGE		0x03		// 改ページする

// メッセージウィンドウの状態 //
#define MWIN_DEAD		0x00			// 使用されていない
#define MWIN_OPEN		0x01			// オープン中
#define MWIN_CLOSE		0x02			// クローズ中
#define MWIN_FREE		0x03			// 待ち状態

// メッセージウィンドウ(顔の状態) //
#define MFACE_NONE		0x00			// 表示されていない
#define MFACE_OPEN		0x01			// オープン中
#define MFACE_CLOSE		0x02			// クローズ中
#define MFACE_NEXT		0x03			// 次の顔へ
#define MFACE_WAIT		0x04			// 待ち状態

// その他の定数 //
#define CWIN_KEYWAIT	8



///// [構造体] /////

// 子ウィンドウの情報 //
struct WINDOW_INFO {
	enum class STATE : uint8_t {
		REGULAR = 0,
		HIGHLIGHT = 1,
		DISABLED = 2,
		COUNT,
	};

	Narrow::literal	Title;	// タイトル文字列へのポインタ(実体ではない！)
	Narrow::literal	Help;	// ヘルプ文字列へのポインタ(これも実体ではない)

	// 特殊処理用コールバック関数(未使用ならNULL)
	bool	(*CallBackFn)(INPUT_BITS);

	STATE	State = STATE::REGULAR;

	// Required for forcing the item to be re-rendered after a state change.
	STATE	StatePrev = STATE::COUNT;

	uint8_t	NumItems;	// 項目数(<ITEM_MAX)
	WINDOW_INFO*	ItemPtr[WINITEM_MAX];	// 次の項目へのポインタ

	constexpr WINDOW_INFO(
		const Narrow::literal title = "",
		const Narrow::literal help = "",
		decltype(CallBackFn) callback_fn = nullptr
	) :
		Title(title), Help(help), CallBackFn(callback_fn), NumItems(0)
	{
		if(!callback_fn) {
			State = STATE::DISABLED;
		}
	}

	constexpr WINDOW_INFO(
		const Narrow::literal title,
		const Narrow::literal help,
		std::span<WINDOW_INFO> children
	) :
		Title(title), Help(help), CallBackFn(nullptr), NumItems(children.size())
	{
		for(size_t i = 0; auto& item : children) {
			ItemPtr[i++] = &item;
		}
	}

	// Returns the maximum number of items among all submenus.
	uint8_t MaxItems() const;

	void SetActive(bool active);
};

// ウィンドウ群 //
typedef struct tagWINDOW_SYSTEM{
	WINDOW_INFO		Parent;					// 親ウィンドウ
	int				x,y;					// ウィンドウ左上の座標
	PIXEL_COORD	W;
	uint32_t	Count;	// フレームカウンタ
	uint8_t	Select[WINDOW_DEPTH];	// 選択中の項目スタック
	uint8_t	SelectDepth;	// 選択中の項目に対するＳＰ
	uint8_t	State;	// 状態

	INPUT_BITS	OldKey;	// 前に押されていたキー
	uint8_t	KeyCount;	// キーボードウェイト
	bool	FirstWait;	// 最初のキー解放待ち

	TEXTRENDER_RECT_ID	TRRs[1 + WINITEM_MAX]; // Initialized by Init().

	// Prepares text rendering for a window with the given width.
	void Init(PIXEL_COORD w);

	// コマンドウィンドウの初期化 //
	void Open(WINDOW_POINT topleft, int select);

	void OpenCentered(PIXEL_COORD w, int select);
} WINDOW_SYSTEM;



// Turns [Sys] into a vertically scrolling window, with elements generated on
// the fly.
// Unfortunately has to be a template because [WINDOW_SYSTEM::CallBackFn]
// doesn't take a reference to a window system object.
template <
	WINDOW_SYSTEM& Sys,
	size_t (*ListSize)(),
	void (*Generate)(WINDOW_INFO& ret, size_t generated, size_t selected),
	bool (*Handle)(INPUT_BITS key, size_t selected)
> class WINDOW_SCROLL {
	// Rewritten when scrolling.
	static inline WINDOW_INFO Item[WINITEM_MAX] = {};

	static inline size_t Sel = 0;
	static inline WINDOW_SYSTEM* ReturnTo = nullptr;

	static void Scroll(void)
	{
		const auto total = ListSize();
		const auto visible = Sys.Parent.NumItems;
		const auto visible_half = (Sys.Parent.NumItems / 2);
		size_t generated_i = (
			(Sel < visible_half) ? 0 :
			(Sel >= (total - visible_half)) ? (total - visible) :
			(Sel - visible_half)
		);
		for(auto item_i = decltype(visible){0}; item_i < visible; item_i++) {
			Item[item_i].CallBackFn = Fn;
			Generate(Item[item_i], generated_i, Sel);
			if(generated_i == Sel) {
				Sys.Select[0] = item_i;
			}
			generated_i++;
		}
	}

	static bool Fn(INPUT_BITS key)
	{
		if(key == KEY_UP) {
			if(Sel == 0) {
				Sel = ListSize();
			}
			Sel--;
			Scroll();
		} else if(key == KEY_DOWN) {
			Sel++;
			if(Sel >= ListSize()) {
				Sel = 0;
			}
			Scroll();
		} else if((key == KEY_BOMB) || (key == KEY_ESC)) {
			Sys.State = CWIN_DEAD;
			if(ReturnTo) {
				ReturnTo->OldKey = key;
			}
			return false;
		}
		return Handle(key, Sel);
	}

public:
	static void Init(
		const Narrow::literal title,
		size_t sel,
		PIXEL_COORD w,
		WINDOW_SYSTEM* return_to
	)
	{
		assert(sel < ListSize());

		ReturnTo = return_to;
		Sys.Parent.Title = title;
		Sys.Parent.Help = "";
		Sys.Parent.NumItems = (std::min)(ListSize(), size_t{ WINITEM_MAX });
		Sys.Parent.CallBackFn = Fn;
		for(size_t i = 0; i < Sys.Parent.NumItems; i++) {
			Sys.Parent.ItemPtr[i] = &Item[i];
		}
		Scroll();
		Sys.Init(w);
	}
};



///// [ 関数 ] /////

// コマンドウィンドウ処理 //
void CWinMove(WINDOW_SYSTEM *ws);				// コマンドウィンドウを１フレーム動作させる
void CWinDraw(WINDOW_SYSTEM *ws);				// コマンドウィンドウの描画
bool CWinExitFn(INPUT_BITS key);	// コマンド [Exit] のデフォルト処理関数

// Calculates the rendered width of the given text in the menu item font,
// without any padding.
PIXEL_SIZE CWinTextExtent(Narrow::string_view str);

// Calculates the rendered width of a whole padded menu item with the given
// text.
PIXEL_SIZE CWinItemExtent(Narrow::string_view str);

// メッセージウィンドウ処理 //

enum class MSG_WINDOW_FLAGS : uint8_t {
	NONE = 0x0,
	WITH_FACE = 0x1,	// Pads all text to leave room for a face portrait.
	_HAS_BITFLAG_OPERATORS,
};

// Prepares text rendering for a window with the given dimensions.
void MWinInit(
	const WINDOW_LTRB& rc, MSG_WINDOW_FLAGS flags = MSG_WINDOW_FLAGS::NONE
);

void MWinOpen(void);	// メッセージウィンドウをオープンする
void MWinClose(void);			// メッセージウィンドウをクローズする
void MWinForceClose(void);		// メッセージウィンドウを強制クローズする
void MWinMove(void);			// メッセージウィンドウを動作させる(後で上と統合する)
void MWinDraw(void);			// メッセージウィンドウを描画する(上に同じ)

void MWinMsg(Narrow::string_view str);	// メッセージ文字列を送る
void MWinFace(uint8_t faceID);	// 顔をセットする
void MWinCmd(uint8_t cmd);	// コマンドを送る

void MWinHelp(WINDOW_SYSTEM *ws);		// メッセージウィンドウにヘルプ文字列を送る



#endif
