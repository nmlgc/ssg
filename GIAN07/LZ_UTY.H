/*
 *   Bit stream writer and packfile compression
 *
 *   Reading and decompression is part of the game logic layer.
 */

#pragma once

#include "hatoyama/engine/file.h"
#include "hatoyama/logic/buffer.h"

struct BIT_DEVICE_WRITE {
	BUFFER_GROWABLE buffer;
	uint8_t bit_cursor:3 = 0;

	void PutBit(uint8_t bit);
	void PutBits(uint32_t bits,unsigned int bitcount);
	bool Write(const char8_t *s) const;
};

bool FilWrite(
	const char8_t *s,
	std::span<BUFFER_BORROWED> files,
	std::unique_ptr<FILE_TIMESTAMPS> maybe_timestamps = nullptr
);
