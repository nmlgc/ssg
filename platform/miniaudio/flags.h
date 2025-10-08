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
#ifdef WIN32_VINTAGE
#define MA_NO_WASAPI
#define MA_NO_SSE2
#define MA_NO_AVX2
#endif

// Slightly crispier than what you'd get with DirectSound, but 2 would already
// come with fewer high frequencies in comparison.
#define MA_DEFAULT_RESAMPLER_LPF_ORDER 1
