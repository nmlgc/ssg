/*
 *   Generic entity management
 *
 */

#pragma once

#include <array>
#include <algorithm>

template <class T, size_t N, typename ShouldDelete> void Indsort(
	std::array<uint16_t, N>& indices,
	uint16_t& count,
	const std::array<T, N>& entities,
	ShouldDelete should_delete
)
{
	uint16_t i;
	uint16_t next;

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
			std::swap(indices[i], indices[temp]);
		} else {
			next++;
		}
	}
	count = next;
}
