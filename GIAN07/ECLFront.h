/*
 *   Frontend ECL handler
 *
 */

#pragma once

#include "hatoyama/logic/ffi.h"

struct ENEMY_DATA;
struct HOOKS;

void ECL_Frontend(
	const uint8_t *cmd, const ENEMY_DATA *, bool8_t error, HOOKS *
);
