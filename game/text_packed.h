/*
 *   Rectangle management for text rendering by packing each rectangle into a
 *   larger surface
 *
 */

#pragma once

#include "game/text.h"

class TEXTRENDER_PACKED {
protected:
	struct RECT_AND_CONTENTS {
		PIXEL_LTWH rect;
		std::optional<Narrow::string> contents;
	};

	PIXEL_SIZE bounds = {};
	std::vector<PIXEL_LTWH> spaces;
	std::vector<RECT_AND_CONTENTS> rects;

	template <class T> void SpaceAdd(T&& space) {
		if((space.w > 0) && (space.h > 0)) {
			spaces.emplace_back(std::forward<T>(space));
		}
	}

	// Inserts a rectangle of the given size, expanding the empty space as
	// needed.
	PIXEL_LTWH Insert(const PIXEL_SIZE& subrect_size);

public:
	PIXEL_LTWH Subrect(
		TEXTRENDER_RECT_ID rect_id, std::optional<PIXEL_LTWH> maybe_subrect
	);

	TEXTRENDER_RECT_ID Register(const PIXEL_SIZE& size);

	bool Wipe();

	// Resets both bounds and empty spaces.
	void Clear();

	bool Blit(
		WINDOW_POINT dst,
		TEXTRENDER_RECT_ID rect_id,
		std::optional<PIXEL_LTWH> subrect = std::nullopt
	);

	template <typename Self> bool Prerender(
		this Self&& self,
		TEXTRENDER_RECT_ID rect_id,
		std::invocable<TEXTRENDER_SESSION&> auto func
	) {
		auto maybe_session = self.Session(rect_id);
		if(!maybe_session) {
			return false;
		}
		auto& session = maybe_session.value();
		func(session);
		return true;
	}

	template <typename Self> bool Render(
		this Self&& self,
		WINDOW_POINT dst,
		TEXTRENDER_RECT_ID rect_id,
		Narrow::string_view contents,
		std::invocable<TEXTRENDER_SESSION&> auto func,
		std::optional<PIXEL_LTWH> subrect = std::nullopt
	) {
		assert(rect_id < self.rects.size());
		auto& rect = self.rects[rect_id];
		if(rect.contents != contents) {
			if(!self.Prerender(rect_id, func)) {
				return false;
			}
			rect.contents = contents;
		}
		return self.Blit(dst, rect_id, subrect);
	}
};
