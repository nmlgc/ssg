/*
 *   Logic part of the SCL parser
 *
 */

#include "ssg/internal/Stage.hpp"

BUFFER_OWNED SCL_Head = nullptr;
uint8_t *SCL_Now = nullptr;

// ＳＣＬに関する情報
SCL_INFO SclInfo;
