/*
 *   The linear congruential generator from Borland C/C++
 *
 *   https://en.wikipedia.org/w/index.php?title=Linear_congruential_generator&oldid=1337691900#Parameters_in_common_use
 */

#pragma once

import std.compat;

// Instantiable version of the RNG used in Shuusou Gyoku
struct C_RNG {
	static constexpr uint32_t RAND_A = 22695477; // 0x015a4e35

	uint32_t seed = 0; // 乱数のたね //

	uint16_t next(void) {
		seed = ((seed * RAND_A) + 1);
		return ((seed >> 16) & 0x7FFF);
	}
};
