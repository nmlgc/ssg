/*
 *   Volume handling
 *
 */

#pragma once

#include <assert.h>
#include <stdint.h>

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
