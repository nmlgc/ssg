/*
 *   Logic part of the SCL parser
 *
 */

#include "ssg/internal/Stage.hpp"

C_STAGE Stage;

bool C_STAGE::Set(BUFFER_OWNED&& data)
{
	SCL_Head = std::move(data);
	SCL_Now = SCL_Head.get();
	GameCount = 0;
	SclInfo.MsgFlag = false;
	SclInfo.ReturnFlag = false;

	return (SCL_Head.get() != nullptr);
}
