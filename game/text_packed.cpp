/*
 *   Rectangle management for text rendering by packing each rectangle into a
 *   larger surface
 *
 *   The rectangle packing algorithm was adapted and simplified from
 *
 *   	https://github.com/TeamHypersomnia/rectpack2D
 *
 *   and enhanced with expansion into either direction.
 */

#include "game/text_packed.h"
#include "platform/graphics_backend.h"
#include <assert.h>

struct created_splits {
	int count = 0;
	std::array<PIXEL_LTWH, 2> spaces;

	static auto failed() {
		created_splits result;
		result.count = -1;
		return result;
	}

	static auto none() {
		return created_splits();
	}

	template <class... Args> created_splits(Args&&... args) noexcept :
		count(sizeof...(Args)),
		spaces({ std::forward<Args>(args)... }) {
	}

	explicit operator bool() const {
		return count != -1;
	}
};

created_splits insert_and_split(const PIXEL_SIZE& nw, const PIXEL_LTWH& sp)
{
	const auto free_w = (sp.w - nw.w);
	const auto free_h = (sp.h - nw.h);

	if(free_w < 0 || free_h < 0) {
		// Image is bigger than the candidate empty space.
		// We'll need to look further.
		return created_splits::failed();
	}

	if((free_w == 0) && (free_h == 0)) {
		// If the image dimensions equal the dimensions of the candidate empty
		// space (image fits exactly), we will just delete the space and create
		// no splits.
		return created_splits::none();
	}

	// If the image fits into the candidate empty space, but exactly one of the
	// image dimensions equals the respective dimension of the candidate empty
	// space (e.g. image = 20x40, candidate space = 30x40) we delete the space
	// and create a single split. In this case a 10x40 space.
	if((free_w > 0) && (free_h == 0)) {
		auto r = sp;
		r.left += nw.w;
		r.w -= nw.w;
		return created_splits(r);
	}
	if((free_w == 0) && (free_h > 0)) {
		auto r = sp;
		r.top += nw.h;
		r.h -= nw.h;
		return created_splits(r);
	}

	// Every other option has been exhausted, so at this point the image must
	// be *strictly* smaller than the empty space, that is, it is smaller in
	// both width and height.
	// Thus, free_w and free_h must be positive.

	// Decide which way to split.
	//
	// Instead of having two normally-sized spaces, it is better - though I
	// have no proof of that - to have a one tiny space and a one huge space.
	// This creates better opportunity for insertion of future rectangles.
	//
	// This is why, if we had more of width remaining than we had of height,
	// we split along the vertical axis, and if we had more of height remaining
	// than we had of width, we split along the horizontal axis.
	PIXEL_LTWH bigger_split;
	PIXEL_LTWH lesser_split;
	if(free_w > free_h) {
		bigger_split = { (sp.left + nw.w), sp.top, free_w, sp.h };
		lesser_split = { sp.left, (sp.top + nw.h), nw.w, free_h };
	} else {
		bigger_split = { sp.left, (sp.top + nw.h), sp.w, free_h };
		lesser_split = { (sp.left + nw.w), sp.top, free_w, nw.h };
	}
	return created_splits(bigger_split, lesser_split);
}

PIXEL_LTWH TEXTRENDER_PACKED_BASE::Insert(const PIXEL_SIZE& subrect_size)
{
	PIXEL_LTWH* closest = nullptr;

	assert(subrect_size);
	for(int i = static_cast<int>(spaces.size()) - 1; i >= 0; --i) {
		const PIXEL_LTWH candidate = spaces[i];

		if(!closest || (
			(candidate.w * candidate.h) < (closest->w * closest->h)
		)) {
			closest = &spaces[i];
		}

		const auto splits = insert_and_split(subrect_size, candidate);
		if(splits) {
			spaces[i] = spaces.back();
			spaces.pop_back();

			for (int s = 0; s < splits.count; ++s) {
				SpaceAdd(splits.spaces[s]);
			}

			const PIXEL_LTWH ret = {
				candidate.left, candidate.top, subrect_size.w, subrect_size.h
			};
			bounds.w = std::max(bounds.w, (ret.left + ret.w));
			bounds.h = std::max(bounds.h, (ret.top  + ret.h));
			return ret;
		}
	}

	// Expand the closest space in-place, but only if this would add fewer
	// pixels than starting a new row or column. The bounds are resized
	// accordingly during the actual insertion.
	if(
		closest &&
		((subrect_size.w - closest->w) < subrect_size.h) &&
		((subrect_size.h - closest->h) < subrect_size.w)
	) {
		closest->w = subrect_size.w;
		closest->h = subrect_size.h;
	} else {
		constexpr auto coord_max = std::numeric_limits<PIXEL_COORD>::max();

		if(bounds.w <= bounds.h) {
			assert(subrect_size.w <= (coord_max - bounds.w));
			SpaceAdd(PIXEL_LTWH{
				bounds.w, 0, subrect_size.w, std::max(bounds.h, subrect_size.h)
			});
		} else {
			assert(subrect_size.h <= (coord_max - bounds.h));
			SpaceAdd(PIXEL_LTWH{
				0, bounds.h, std::max(bounds.w, subrect_size.w), subrect_size.h
			});
		}
	}
	// Might as well recurse for the assignment of the resulting rectangle to
	// simplify the code.
	return Insert(subrect_size);
}

PIXEL_LTWH TEXTRENDER_PACKED_BASE::Subrect(
	TEXTRENDER_RECT_ID rect_id, std::optional<PIXEL_LTWH> maybe_subrect
) {
	assert(rect_id < rects.size());
	auto ret = rects[rect_id].rect;
	if(maybe_subrect) {
		const auto& subrect = maybe_subrect.value();
		ret.left += subrect.left;
		ret.top += subrect.top;
		ret.w = (std::min)(subrect.w, ret.w);
		ret.h = (std::min)(subrect.h, ret.h);
	}
	return ret;
}

TEXTRENDER_RECT_ID TEXTRENDER_PACKED_BASE::Register(const PIXEL_SIZE& size)
{
	rects.emplace_back(Insert(size));
	return static_cast<TEXTRENDER_RECT_ID>(rects.size() - 1);
}

bool TEXTRENDER_PACKED_BASE::Wipe()
{
	for(auto& rect : rects) {
		rect.contents = std::nullopt;
	}
	return true;
}

void TEXTRENDER_PACKED_BASE::Clear()
{
	bounds = {};
	spaces.clear();
	rects.clear();
}

bool TEXTRENDER_PACKED_BASE::Blit(
	WINDOW_POINT dst,
	TEXTRENDER_RECT_ID rect_id,
	std::optional<PIXEL_LTWH> subrect
)
{
	const PIXEL_LTRB rect = Subrect(rect_id, subrect);
	return GrpSurface_Blit(dst, SURFACE_ID::TEXT, rect);
}
