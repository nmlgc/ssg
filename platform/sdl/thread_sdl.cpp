/*
 *   Threads via SDL
 *
 */

#include <SDL_thread.h>
#include "platform/sdl/thread_sdl.h"

THREAD::THREAD(SDL_Thread *sdl_thread, std::unique_ptr<THREAD_STOP> st) :
	sdl_thread(sdl_thread), st(std::move(st))
{
}

THREAD::THREAD(THREAD&& other) noexcept
{
	*this = std::move(other);
}

THREAD& THREAD::operator=(THREAD&& other) noexcept
{
	// Must turn this instance to an empty thread to avoid double-frees when
	// calling the destructor.
	sdl_thread = std::exchange(other.sdl_thread, nullptr);
	st = std::exchange(other.st, nullptr);
	return *this;
};

THREAD::~THREAD()
{
	if(st) {
		st->store(true);
	}
	Join();
}

SDL_Thread *HelpCreateThread(int(*fn)(void *), void *data)
{
	return SDL_CreateThread(fn, "", data);
}

bool THREAD::Joinable() const noexcept
{
	return (sdl_thread != nullptr);
}

void THREAD::Join() noexcept
{
	if(sdl_thread) {
		SDL_WaitThread(sdl_thread, nullptr);
		sdl_thread = nullptr;
	}
}
