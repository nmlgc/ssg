/*                                                                           */
/*   FontUty.h   フォントの処理いろいろ                                      */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_FONTUTY_H
#define PBGWIN_FONTUTY_H		"FONTUTY : Version 0.02 : Update 2000/07/22"
//#pragma message(PBGWIN_FONTUTY_H)



///// [更新履歴] /////

// 2000/07/22 : フォント追加に伴い、プログラムの一部を変更
// 2000/02/19 : フォントに関する処理の開発を始める



#include "DirectXUTYs/DD_UTY.H"
#include "platform/windows/text_gdi.h"
#include <string_view>



enum class GIAN_FONT_ID : uint8_t {
	SMALL = 0,	// フォント(小さい文字用)
	NORMAL = 1,	// フォント(通常の文字用)
	LARGE = 2,	// フォント(大きい文字用)
	COUNT,
};

struct GRAPHICS_DDRAW;
struct SURFACE_DDRAW;
extern TEXTRENDER_GDI<GRAPHICS_DDRAW, SURFACE_DDRAW, GIAN_FONT_ID> TextObj;
template <class T> concept GIAN_TEXTRENDER_SESSION = (
	TEXTRENDER_SESSION<T, GIAN_FONT_ID>
);
static_assert(TEXTRENDER<decltype(TextObj), GIAN_FONT_ID>);



///// [ 関数 ] /////
extern void GrpPut16(int x, int y, const char *s);	// 16x16 透過フォントで文字列出力(高速)
extern void GrpPut16c(int x, int y, const char *s);	// 16x16 透過フォントで文字列出力(ｸﾘｯﾋﾟﾝｸﾞ有)
extern void GrpPut16c2(int x, int y, const char *s);	// 上と同じだが、ｘ移動幅が１６
extern void GrpPutc(int x, int y, char c);	// 16x16 透過フォントで文字出力(ｸﾘｯﾋﾟﾝｸﾞ有)
extern void GrpPut57(int x, int y, const char *s);	// 05x07 べた貼りフォント
extern void GrpPut7B(int x, int y, const char *s);	// 07x11 音楽室用フォント
extern void GrpPutScore(int x, int y, const char *s);	// 得点アイテムのスコアを描画

extern void GrpPutMidNum(int x, int y, int n);	// MIDI 用フォントを描画する

// グラデーション付きフォントを描画する
PIXEL_SIZE DrawGrdFont(
	TEXTRENDER_RECT_ID rect_id, std::span<const Narrow::string_view> strs
);


#endif
