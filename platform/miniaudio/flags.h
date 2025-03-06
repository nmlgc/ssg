/*
 *   miniaudio configuration
 *
 */

#pragma once

#define MA_NO_ENCODING
#define MA_NO_GENERATION
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_FLAC
#define MA_NO_MP3
#define MA_NO_NULL

// Slightly crispier than what you'd get with DirectSound, but 2 would already
// come with fewer high frequencies in comparison.
#define MA_DEFAULT_RESAMPLER_LPF_ORDER 1
