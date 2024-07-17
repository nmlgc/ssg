/*
 *   Time interface (std::chrono implementation)
 *
 */

#include "platform/time.h"
#include <chrono>

uint32_t Time_SteadyTicksMS()
{
	const auto now = (std::chrono::steady_clock::now().time_since_epoch());
	return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}
