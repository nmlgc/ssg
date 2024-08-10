/*
 *   Generic, cross-platform subsystem initialization and cleanup
 *
 */

#pragma once

#include "CONFIG.H"

bool XInit(void);
void XCleanup(void);

void XGrpTry(const GRAPHICS_PARAMS& params);

// Tries the graphics configuration that results from applying the given
// [patch_func] onto the current configuration, and updates all subsystems
// accordingly.
void XGrpTry(std::invocable<GRAPHICS_PARAMS&> auto&& patch_func) {
	auto params = ConfigDat.GraphicsParams();
	patch_func(params);
	XGrpTry(params);
}
