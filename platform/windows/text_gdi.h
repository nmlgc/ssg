/*
 *   Text rendering via GDI
 *
 */

#pragma once

#define WIN32_LEAN_AND_MEAN

#include "game/enum_array.h"
#include "game/text_packed.h"
#include <windows.h>

template <
	class Surface, ENUMARRAY_ID FontID
> class TEXTRENDER_GDI : public TEXTRENDER_PACKED {
	Surface& surf;

public:
	const ENUMARRAY<HFONT, FontID>& fonts;

	// Can share the font array with other GDI renderers.
	TEXTRENDER_GDI(Surface& surf, const decltype(fonts)& fonts) :
		surf(surf), fonts(fonts) {
	}
};
