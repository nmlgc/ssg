/*
 *   String formatting helpers
 *
 */

#pragma once

// Number of decimal digits required to store the highest value of the given
// type.
template <class T> constexpr auto STRING_NUM_CAP = (
	((241 * sizeof(T)) / 100) + 1
);
