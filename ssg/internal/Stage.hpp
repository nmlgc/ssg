/*
 *   SCL parser
 *
 */

#pragma once

#include "hatoyama/logic/buffer.h"

// ＳＣＬ管理用構造体 //
typedef struct {
	// メッセージスキップ用フラグ
	bool MsgFlag;

	// リターンキー用フラグ
	bool ReturnFlag;
} SCL_INFO;

// ＳＣＬに関する情報
extern SCL_INFO SclInfo;

extern BUFFER_OWNED SCL_Head;
extern uint8_t *SCL_Now;
extern uint32_t GameCount;

void StageMove(void);
