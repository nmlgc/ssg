/*
 *   Common paths as determined by SDL's path functions
 *
 */

// SDL headers must come first to avoid import→#include bugs on Clang 19.
#ifdef SDL3
#include <SDL3/SDL_filesystem.h>
#else
#include <SDL_filesystem.h>
#endif

#include "platform/path.h"
#include "constants.h"

#if (!defined(WIN32) || !defined(SDL3))
	constexpr auto SDL_free_deleter = [](auto* p) { SDL_free(p); };
	static std::unique_ptr<char8_t[], decltype(SDL_free_deleter)> PathData = {
		nullptr
	};
#endif
static std::u8string_view PathDataView;

std::u8string_view PathForData(void)
{
	if(PathDataView.data() != nullptr) {
		return PathDataView;
	}
#ifdef WIN32
#ifdef SDL3
	PathDataView = std::bit_cast<const char8_t *>(SDL_GetBasePath());
#else
	PathData.reset(std::bit_cast<char8_t *>(SDL_GetBasePath()));
	PathDataView = PathData.get();
#endif
#else
#ifdef PATH_XDG_DATA_HOME_HAS_APP_ID
	auto* path_base = SDL_GetPrefPath(nullptr, "");
#else
	auto* path_base = SDL_GetPrefPath(GAME_ORG, GAME_APP);
#endif
	PathData.reset(std::bit_cast<char8_t *>(path_base));
	PathDataView = PathData.get();

	// Remove the unsightly second slash in case we passed an empty app
	if(PathDataView.ends_with(u8"//")) {
		PathDataView.remove_suffix(1);
	}
#endif

	return PathDataView;
}
