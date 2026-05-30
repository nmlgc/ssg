/*
 *   Generic entity management
 *
 */

#pragma once

import std.compat;
#include <assert.h>

template <size_t N> using IND_TYPE = std::conditional_t<
	(N <= (std::numeric_limits<uint8_t>::max)()),
	uint8_t,
	uint16_t
>;

template <class T, size_t N, typename ShouldDelete> void Indsort(
	std::array<IND_TYPE<N>, N>& indices,
	IND_TYPE<N>& count,
	const std::array<T, N>& entities,
	ShouldDelete should_delete
)
{
	IND_TYPE<N> i;
	IND_TYPE<N> next;

	for(i = next = 0; i < count; i++) {
		// 消去要請フラグが立っている->swap 立っていない-> counter++ //
		if(should_delete(entities[indices[i]])) {
			uint16_t temp;
			// フラグの立っていないアイテムを検索する //
			for(temp = (i + 1); temp < count; temp++) {
				if(should_delete(entities[indices[temp]]) == 0){
					next++;
					break;
				}
			}

			// pbg landmine: This looks as if it will write out of bounds on
			// the last iteration of the loop ([i] == ([count] - 1). It only
			// doesn't because every [count]-mutating setter function ensures
			// that it stays ≤([N] - 1), thus reducing the effective [N] by 1.
			assert(
				(count <= (N - 1)) ||
				!"setter function violated entity cap precondition"
			);
			#pragma warning(suppress: 28020)
			std::swap(indices[i], indices[temp]);
		} else {
			next++;
		}
	}
	count = next;
}
