/*
 *   Platform-specific time interface
 *
 */

import std.compat;

struct TIME_OF_DAY {
	uint32_t year;
	uint8_t month;	// 1-based
	uint8_t day;	// 1-based
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
};

// Returns some kind of steady system tick value in milliseconds.
uint32_t Time_SteadyTicksMS();

// Returns the current local system time.
TIME_OF_DAY Time_NowLocal();
