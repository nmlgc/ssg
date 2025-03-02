/*
 *   Bit depths and pixel formats
 *
 */

#pragma once

import std.compat;

struct PIXELFORMAT {
	enum FORMAT {
		PALETTE8,
		ANY16,
		ANY32,
	} format;

	enum SIZE {
		SIZE8 = 1,
		SIZE16 = 2,
		SIZE32 = 4,
	};

	bool IsPalettized() const {
		return (format == PALETTE8);
	}

	bool IsChanneled() const {
		return !IsPalettized();
	}

	SIZE PixelSize() const {
		switch(format) {
		case PALETTE8:	return SIZE8;
		case ANY16:   	return SIZE16;
		case ANY32:   	return SIZE32;
		}
		std::unreachable();
	}

	size_t PixelByteSize() const {
		return static_cast<size_t>(PixelSize());
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

		std::optional<PIXELFORMAT> pixel_format() const {
			using F = PIXELFORMAT;
			switch(bpp) {
			case 8: 	return std::make_optional<PIXELFORMAT>(F::PALETTE8);
			case 16:	return std::make_optional<PIXELFORMAT>(F::ANY16);
			case 32:	return std::make_optional<PIXELFORMAT>(F::ANY32);
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
