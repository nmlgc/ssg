/*
 *   Bit depths and pixel formats
 *
 */

#pragma once

import std.compat;

// (We don't really care about the value of whatever variant type this holds,
// just about the type itself.)
struct PIXELFORMAT : public std::variant<uint8_t, uint16_t, uint32_t> {
	// Not worth auto-generating at all.
	using LARGEST = uint32_t;

	bool IsPalettized() const {
		return std::holds_alternative<uint8_t>(*this);
	}

	bool IsChanneled() const {
		return !IsPalettized();
	}

	std::strong_ordering operator <=>(const PIXELFORMAT& other) const {
		return (index() <=> other.index());
	}
};

class BITDEPTHS {
protected:
	// Since this class is directly serialized, an enum would be actively
	// harmful here.
	using ARRAY = std::array<uint8_t, 3>;

	// Sorted from the lowest to the highest one.
	constexpr static const ARRAY SUPPORTED = { 8, 16, 32 };
	static_assert(SUPPORTED[0] < SUPPORTED[1]);
	static_assert(SUPPORTED[1] < SUPPORTED[2]);

public:
	class BITDEPTH {
		uint8_t bpp;

		ARRAY::const_iterator find() const {
			return std::ranges::find(SUPPORTED, bpp);
		}

	public:
		constexpr BITDEPTH() noexcept :
			bpp(0) {
		}
		constexpr BITDEPTH(const BITDEPTH& other) = default;
		constexpr BITDEPTH& operator=(const BITDEPTH& other) = default;
		constexpr std::strong_ordering operator<=>(
			const BITDEPTH& o
		) const = default;

		constexpr BITDEPTH(ARRAY::const_iterator it) :
			bpp((it >= SUPPORTED.end()) ? 0 : *it) {
		}

		explicit operator bool() const {
			return (find() != SUPPORTED.end());
		}

		uint8_t value() const {
			return bpp;
		}

		// Adding std::monostate to PIXELFORMAT would just make it awful to
		// use.
		std::optional<PIXELFORMAT> pixel_format() const {
			switch(bpp) {
			case  8:	return std::make_optional<PIXELFORMAT>(uint8_t{});
			case 16:	return std::make_optional<PIXELFORMAT>(uint16_t{});
			case 32:	return std::make_optional<PIXELFORMAT>(uint32_t{});
			default:	return std::nullopt;
			}
		}

		// Cycles through all supported bit depths.
		BITDEPTH cycle(bool reverse) const {
			const auto it = find();
			// Visual Studio raises a debug exception if an iterator is
			// decremented past begin()...
			if(it == SUPPORTED.end()) {
				return {};
			} else if(reverse && (it == SUPPORTED.begin())) {
				return (SUPPORTED.end() - 1);
			} else if(!reverse && (it == (SUPPORTED.end() - 1))) {
				return SUPPORTED.begin();
			}
			return (it + (reverse ? -1 : +1));
		}
	};

	static_assert(
		sizeof(BITDEPTH) == sizeof(uint8_t),
		"The BITDEPTH class is serialized and must be exactly 1 byte large."
	);

	constexpr static BITDEPTH find_if(auto pred) {
		return std::ranges::find_if(SUPPORTED, pred);
	}

	constexpr static BITDEPTH find(uint8_t bpp) {
		return std::ranges::find(SUPPORTED, bpp);
	}
};

using BITDEPTH = BITDEPTHS::BITDEPTH;
