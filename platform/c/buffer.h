/*
 *   Memory ownership semantics
 *
 */

#pragma once

#include <memory>
#include <span>
#include <vector>

using BYTE_BUFFER_BORROWED = std::span<const uint8_t>;
using BYTE_BUFFER_OWNED = std::unique_ptr<uint8_t[]>;
using BYTE_BUFFER_GROWABLE = std::vector<uint8_t>;
