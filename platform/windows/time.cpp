/*
 *   Time interface (Win32 implementation)
 *
 */

#include "platform/time.h"
#include "game/cast.h"
#include <windows.h>

uint32_t Time_SteadyTicksMS()
{
	return timeGetTime();
}

TIME_OF_DAY Time_NowLocal()
{
	SYSTEMTIME systime;
	GetLocalTime(&systime);
	return TIME_OF_DAY{
		.year   = systime.wYear,
		.month  = Cast::down<uint8_t>(systime.wMonth),
		.day    = Cast::down<uint8_t>(systime.wDay),
		.hour   = Cast::down<uint8_t>(systime.wHour),
		.minute = Cast::down<uint8_t>(systime.wMinute),
		.second = Cast::down<uint8_t>(systime.wSecond),
	};
}
