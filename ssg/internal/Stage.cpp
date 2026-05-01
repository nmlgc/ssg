/*
 *   Logic part of the SCL parser
 *
 */

#include "ssg/internal/Stage.hpp"
#include "EFFECT3D.H"

C_STAGE Stage;

bool C_STAGE::Set(BUFFER_OWNED&& data)
{
	SCL_Head = std::move(data);
	SCL_Now = SCL_Head.get();
	GameCount = 0;
	SclInfo.MsgFlag = false;
	SclInfo.ReturnFlag = false;
	Effect3D.Set(EFFECT3D_TYPE::NONE);

	return (SCL_Head.get() != nullptr);
}
