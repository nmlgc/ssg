/*
 *   SCL parser
 *
 */

#pragma once

// ＳＣＬ管理用構造体 //
typedef struct {
	// メッセージスキップ用フラグ
	bool MsgFlag;

	// リターンキー用フラグ
	bool ReturnFlag;
} SCL_INFO;

// ＳＣＬに関する情報
extern SCL_INFO SclInfo;
