/*
 *   Rectangle management for text rendering by packing each rectangle into a
 *   larger surface
 *
 */

#pragma once

#include "game/text.h"
#include <optional>
#include <vector>

class TEXTRENDER_PACKED {
protected:
	PIXEL_SIZE bounds = {};
	std::vector<PIXEL_LTWH> spaces;
	std::vector<PIXEL_LTWH> rects;

	template <class T> void SpaceAdd(T&& space) {
		if((space.w > 0) && (space.h > 0)) {
			spaces.emplace_back(std::forward<T>(space));
		}
	}

	// Inserts a rectangle of the given size, expanding the empty space as
	// needed.
	PIXEL_LTWH Insert(const PIXEL_SIZE& subrect_size);

public:
	TEXTRENDER_RECT_ID Register(const PIXEL_SIZE& size);

	// Resets both bounds and empty spaces.
	void Clear();
};
