/*
 *   SCL parser
 *
 */

#pragma once

#include "ssg/Input.h"
#include "hatoyama/logic/buffer.h"

// ＳＣＬ管理用構造体 //
struct SCL_INFO {
	// メッセージスキップ用フラグ
	bool MsgFlag;

	// リターンキー用フラグ
	bool ReturnFlag;
};

class C_STAGE {
private:
	BUFFER_OWNED SCL_Head;
	const uint8_t *SCL_Now;
	uint32_t GameCount;

	// ＳＣＬに関する情報
	SCL_INFO SclInfo;

public:
	// Returns `true` if [data] is a valid pointer.
	bool Set(BUFFER_OWNED&& data);

	void Move(INPUT_BITS input);

	auto SCLCount(void) const {
		return GameCount;
	}

	bool InMsg(void) const {
		return SclInfo.MsgFlag;
	}
};

extern C_STAGE Stage;
