/*
 *   URL opener
 *
 */

#include "platform/urlopen.h"
#ifdef SDL3
#include <SDL3/SDL_misc.h>
#else
#include <SDL_misc.h>
#endif

void URLOpen(const char* url)
{
	SDL_OpenURL(url);
}
