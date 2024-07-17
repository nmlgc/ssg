/*
 *   Time interface (Win32 implementation)
 *
 */

#include "platform/time.h"
#include <windows.h>

uint32_t Time_SteadyTicksMS()
{
	return timeGetTime();
}
