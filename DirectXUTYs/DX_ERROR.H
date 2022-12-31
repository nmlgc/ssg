/*                                                                           */
/*   DX_ERROR.h   DirectX のエラー出力用関数                                 */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_DX_ERROR_H
#define PBGWIN_DX_ERROR_H		"DX_ERROR : Version 0.01 : Update 1999/11/20"

#include <string_view>

// 更新履歴 //

// 1999/12/10 : ファイルにエラー出力をする関数を追加
// 1999/11/20 : 作成開始


// 関数プロトタイプ宣言 //
extern void DebugSetup(const char *filename);	// エラー出力準備(->LogFile)
extern void DebugCleanup(void);	// エラー吐き出し用ファイルを閉じる
extern void DebugOut(std::string_view s);	// デバッグメッセージ吐き出し

#endif
