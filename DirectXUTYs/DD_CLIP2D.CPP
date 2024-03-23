/*                                                                           */
/*   DD_CLIP2D.cpp   ２Ｄ図形描画用クリッピング                              */
/*                                                                           */
/*                                                                           */

#include "DD_CLIP2D.H"
#include <algorithm>
#include <array>
#pragma message(PBGWIN_DD_CLIP2D_H)


enum OUT_EDGE {
	NONE, MIN, MAX
};

extern int CLIP_X_MIN, CLIP_X_MAX, CLIP_Y_MIN, CLIP_Y_MAX;


std::span<const WINDOW_POINT> Grp_PClip(
	std::span<WINDOW_POINT> temp, std::span<const WINDOW_POINT> src
)
{
	std::array<WINDOW_POINT, 20> work; // 結果の点の個数(ダミーです)
	OUT_EDGE lastout;

	auto left = src[0].x; // 最も左の座標の初期化
	auto right = left; // 最も右の座標の初期化
	auto top = src[0].y; // 最も上の座標の初期化
	auto bottom = top; // 最も下の座標の初期化

	for(size_t i = 1; i < src.size(); i++) {
		left = (std::min)(left, src[i].x);
		right = (std::max)(right, src[i].x);
		top = (std::min)(top, src[i].y);
		bottom = (std::max)(bottom, src[i].y);
	};

	// 完全に外側の場合
	if(
		(CLIP_X_MIN > right) || (left > CLIP_X_MAX) ||
		(CLIP_Y_MAX < top) || (CLIP_Y_MIN > bottom)
	) {
		return {};
	}

	// 完全に内側の場合
	if(
		(left >= CLIP_X_MIN) && (right <= CLIP_X_MAX) &&
		(top >= CLIP_Y_MIN) && (bottom <= CLIP_Y_MAX)
	) {
		return src;
	}

	// Grp_PClip のクリップ用マクロ
	const auto getout = [](WINDOW_COORD v, WINDOW_COORD min, WINDOW_COORD max) {
		if(v < min) {
			return OUT_EDGE::MIN;
		} else if(v > max) {
			return OUT_EDGE::MAX;
		}
		return OUT_EDGE::NONE;
	};

	auto cur = src[0];
	lastout = getout(cur.y, CLIP_Y_MIN, CLIP_Y_MAX);
	auto* work_p = work.data();

	for(int src_i = (src.size() - 1); src_i >= 0; src_i--) {
		auto prev = cur;
		cur = src[src_i];
		if(prev != cur) {
			prev -= cur;
			const auto oc = getout(cur.y, CLIP_Y_MIN, CLIP_Y_MAX);
			if(oc != lastout) {
				if(lastout) {
					const auto edge = ((lastout == OUT_EDGE::MIN)
						? CLIP_Y_MIN
						: CLIP_Y_MAX
					);
					if(edge != cur.y) {
						*(work_p)++ = {
							((((edge - cur.y) * prev.x) / prev.y) + cur.x), edge
						};
					}
				}
				if(oc) {
					const auto edge = ((oc == OUT_EDGE::MIN)
						? CLIP_Y_MIN
						: CLIP_Y_MAX
					);
					if((prev.y + cur.y) != edge) {
						*(work_p)++ = {
							((((edge - cur.y) * prev.x) / prev.y) + cur.x), edge
						};
					}
				}
			}
			if(!oc) {
				*(work_p)++ = cur;
			}
			lastout = oc;
		}
	}
	if(work_p == work.data()) {
		return {};
	}

	cur = work[0];
	lastout = getout(cur.x, CLIP_X_MIN, CLIP_X_MAX);
	auto temp_p = temp.begin();
	while(work_p != work.data()) {
		auto prev = cur;
		cur = *(--work_p);
		prev -= cur;
		const auto oc = getout(cur.x, CLIP_X_MIN, CLIP_X_MAX);
		if(oc != lastout) {
			if(lastout) {
				const auto edge = ((lastout == OUT_EDGE::MIN)
					? CLIP_X_MIN
					: CLIP_X_MAX
				);
				if(edge != cur.x) {
					*(temp_p++) = {
						edge, ((((edge - cur.x) * prev.y) / prev.x) + cur.y)
					};
				}
			}
			if(oc) {
				const auto edge = ((oc == OUT_EDGE::MIN)
					? CLIP_X_MIN
					: CLIP_X_MAX
				);
				if((prev.x + cur.x) != edge) {
					*(temp_p++) = {
						edge, ((((edge - cur.x) * prev.y) / prev.x) + cur.y)
					};
				}
			}
		}
		if(!oc) {
			*(temp_p++) = cur;
		}
		lastout = oc;
	};

	return temp.subspan(0, std::distance(temp.begin(), temp_p));
}
