/*
 *   SCL parser
 *
 */

#pragma once

// ＳＣＬ管理用構造体 //
struct SCL_INFO {
	// メッセージスキップ用フラグ
	bool MsgFlag;

	// リターンキー用フラグ
	bool ReturnFlag;
};

// ＳＣＬに関する情報
extern SCL_INFO SclInfo;
