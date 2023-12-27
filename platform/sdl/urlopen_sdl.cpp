/*
 *   URL opener
 *
 */

#include "platform/urlopen.h"
#include <SDL_misc.h>

void URLOpen(const char* url)
{
	SDL_OpenURL(url);
}
