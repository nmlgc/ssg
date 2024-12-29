/*
 *   Common paths as determined by SDL's path functions
 *
 */

// SDL headers must come first to avoid import→#include bugs on Clang 19.
#include <SDL_filesystem.h>

#include "platform/path.h"
#include "constants.h"

constexpr auto SDL_free_deleter = [](auto* p) { SDL_free(p); };
static std::unique_ptr<char8_t[], decltype(SDL_free_deleter)> PathData = {
	nullptr
};
static std::u8string_view PathDataView;

std::u8string_view PathForData(void)
{
	if(PathDataView.data() != nullptr) {
		return PathDataView;
	}
	#ifdef WIN32
		auto* path_base = SDL_GetBasePath();
	#else
		auto* path_base = SDL_GetPrefPath(GAME_ORG, GAME_APP);
	#endif
	if(!path_base) {
		return {};
	}
	PathData.reset(std::bit_cast<char8_t *>(path_base));
	PathDataView = PathData.get();
	return PathDataView;
}
