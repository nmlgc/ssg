/*
 *   SCL parser
 *
 */

#pragma once

#include "hatoyama/logic/buffer.h"

// ＳＣＬ管理用構造体 //
struct SCL_INFO {
	// メッセージスキップ用フラグ
	bool MsgFlag;

	// リターンキー用フラグ
	bool ReturnFlag;
};

// ＳＣＬに関する情報
extern SCL_INFO SclInfo;

extern BUFFER_OWNED SCL_Head;
extern uint8_t *SCL_Now;
