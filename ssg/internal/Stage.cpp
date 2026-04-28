/*
 *   Logic part of the SCL parser
 *
 */

#include "ssg/internal/Stage.hpp"

BUFFER_OWNED SCL_Head = nullptr;
uint8_t *SCL_Now = nullptr;
uint32_t GameCount;

// ＳＣＬに関する情報
SCL_INFO SclInfo;

bool StageSet(BUFFER_OWNED&& data)
{
	SCL_Head = std::move(data);
	SCL_Now = SCL_Head.get();
	GameCount = 0;
	SclInfo.MsgFlag = false;
	SclInfo.ReturnFlag = false;

	return (SCL_Head.get() != nullptr);
}
