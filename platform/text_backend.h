/*
 *   Platform-specific text rendering backend
 *
 */

#pragma once

#ifdef WIN32
	#include "platform/windows/text_gdi.h"

	extern TEXTRENDER_GDI TextObj;
#endif
