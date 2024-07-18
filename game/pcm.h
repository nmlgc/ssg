/*
 *   Sampling rate / bit depth / channel structure
 *
 */

#pragma once

import std.compat;

enum class PCM_SAMPLE_FORMAT : uint8_t {
	S16 = 2,
	S32 = 4,
};

struct PCM_FORMAT {
	const uint32_t samplingrate;
	const uint16_t channels;
	const PCM_SAMPLE_FORMAT format;

	std::strong_ordering operator <=>(const PCM_FORMAT& other) const = default;

	size_t SampleSize() const {
		const auto byte_depth = std::to_underlying(format);
		return (channels * byte_depth);
	}
};
