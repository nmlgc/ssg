/*
 *   Memory ownership semantics for SDL buffers
 *
 */

#include <SDL3/SDL_stdinc.h>

#include "platform/buffer.h"

void* SDL_malloc_wrap(size_t size)
{
	return SDL_malloc(size);
}

void SDL_FREE_DELETER::operator()(void *p)
{
	SDL_free(p);
}
