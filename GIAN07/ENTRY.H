/*
 *   Generic, cross-platform subsystem initialization and cleanup
 *
 */

#pragma once

#include "CONFIG.H"

bool XInit(void);
void XCleanup(void);

void XGrpTry(const GRAPHICS_PARAMS& prev, GRAPHICS_PARAMS& params);

// Tries the graphics configuration that results from applying the given
// [patch_func] onto the current configuration, and updates all subsystems
// accordingly.
void XGrpTry(std::invocable<GRAPHICS_PARAMS&> auto&& patch_func) {
	const auto prev = ConfigDat.GraphicsParams();
	auto params = prev;
	patch_func(params);
	XGrpTry(prev, params);
}

void XGrpTryCycleScale(int_fast8_t delta);
void XGrpTryCycleDisp(void);
void XGrpTryCycleScMode(void);
