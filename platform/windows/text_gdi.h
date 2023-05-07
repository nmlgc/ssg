/*
 *   Text rendering via GDI
 *
 */

#pragma once

#define WIN32_LEAN_AND_MEAN

#include "game/enum_array.h"
#include <windows.h>

template <ENUMARRAY_ID FontID> class TEXTRENDER_GDI {
public:
	const ENUMARRAY<HFONT, FontID>& fonts;

	// Can share the font array with other GDI renderers.
	TEXTRENDER_GDI(const decltype(fonts)& fonts) :
		fonts(fonts) {
	}
};
