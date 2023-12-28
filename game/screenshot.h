/*
 *   Screenshot filename
 *
 */

#pragma once

#include "platform/file.h"

// Required to enable the screenshot feature as a whole.
void ScreenshotSetPrefix(std::u8string_view prefix);

// Increments the screenshot number to the next file that doesn't exist yet,
// then opens a write stream for that file.
std::unique_ptr<FILE_STREAM_WRITE> ScreenshotNextStream();
