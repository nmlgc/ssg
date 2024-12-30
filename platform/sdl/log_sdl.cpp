/*
 *   Internal logging helpers for the SDL platform
 *
 */

// SDL headers must come first to avoid import→#include bugs on Clang 19.
#include <SDL_messagebox.h>

#include "platform/sdl/log_sdl.h"

const char *log_title = nullptr;
SDL_LogOutputFunction log_default_func;
void *log_default_data;

void SDLCALL LogCriticalAsMessageBox(
	void *userdata, int category, SDL_LogPriority priority, const char *message
)
{
	if(priority == SDL_LOG_PRIORITY_CRITICAL) {
		SDL_ShowSimpleMessageBox(
			SDL_MESSAGEBOX_ERROR, log_title, message, nullptr
		);
	} else {
		log_default_func(log_default_data, category, priority, message);
	}
}

void Log_Init(const char8_t *title)
{
	log_title = reinterpret_cast<const char *>(title);

	// The easiest workaround for SDL_ShowSimpleMessageBox()'s lack of built-in
	// string interpolation: Route errors through the log system instead.
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
	SDL_LogGetOutputFunction(&log_default_func, &log_default_data);
	SDL_LogSetOutputFunction(LogCriticalAsMessageBox, nullptr);
}

void Log_Fail(SDL_LogCategory category, const char *msg)
{
	SDL_LogCritical(category, "%s: %s", msg, SDL_GetError());
}
