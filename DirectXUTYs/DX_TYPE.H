/*                                                                           */
/*   DX_TYPE.h   ＤＸ関連の定数とか                                          */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_DX_TYPE_H
#define PBGWIN_DX_TYPE_H		"DX_TYPE : Version 0.02"


// 更新履歴 //

// 2000/05/15 : 関数呼び出し規約 FINT 等を追加
// 2000/01/30 : バージョン文字列を出力するようにした
// 1999/12/02 : ＤＤ＿ＵＴＹから分離する


// ヘッダファイル //
#include <windows.h>
#include <stdio.h>


// 使用するライブラリ //
#pragma comment(lib,"dxguid.lib") // needed for DirectDraw and Direct3D
#pragma comment(lib,"ddraw.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"winmm.lib")


// ヘッダファイルのバージョンを表示する //
#pragma message("-----[" PBGWIN_DX_TYPE_H " / " __TIMESTAMP__ "]-----")

#if   DIRECTDRAW_VERSION == 0x0700
	#pragma message("      DirectDraw  : Version 7.00")
#elif DIRECTDRAW_VERSION == 0x0600
	#pragma message("      DirectDraw  : Version 6.00")
#elif DIRECTDRAW_VERSION == 0x0500
	#pragma message("      DirectDraw  : Version 5.00")
#elif DIRECTDRAW_VERSION == 0x0300
	#pragma message("      DirectDraw  : Version 3.00")
#endif

#pragma message("------------------------------")


#endif
