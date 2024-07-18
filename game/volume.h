/*
 *   Volume handling
 *
 */

#pragma once

import std.compat;
#include <assert.h>

// Discrete volume values for MIDI and the UI.
using VOLUME = uint8_t;

static constexpr VOLUME VOLUME_MAX = 127;

constexpr float VolumeLinear(VOLUME discrete)
{
	return (discrete / float{ VOLUME_MAX });
}

constexpr VOLUME VolumeDiscrete(float linear)
{
	assert((linear >= 0.0f) && (linear <= 1.0f));
	[[gsl::suppress(es.46)]]
	return ((linear * VOLUME_MAX) + 0.5f);
}

// Maps a linear volume value to decibels, using the simple x² curve used by
// the MIDI standard.
inline const float VolumeDBSquare(float linear)
{
	// Yup, MIDI does *not* use logarithmic curves for volume or expression.
	// The "Volume, Expression & Master Volume Response" section of the spec
	// defines the dB level exactly as `40 × log₁₀(volume / 127)`.  A MIDI
	// volume of 64 therefore turns into -11.9 dB or an amplitude of 0.25,
	// which can also be observed on the Microsoft GS Wavetable Synth.
	return (40.0f * log10f(linear));
}

// Maps a linear volume value to a sample multiplication factor, using the
// simple x² curve used by the MIDI standard.
constexpr float VolumeFactorSquare(float linear)
{
	// The `40 × log₁₀(volume / 127)` curve corresponds to a simple (volume²)
	// when converted back to an amplitude, as outlined by
	//
	// 	https://music.stackexchange.com/a/123067
	//
	// Ironically, the spec then goes on to recommend an exponential function
	// for note *velocity*...
	return (linear * linear);
}

constexpr float VolumeFactorSquare(VOLUME discrete)
{
	return VolumeFactorSquare(VolumeLinear(discrete));
}
