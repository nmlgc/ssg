/*
 *   Common paths as determined by SDL's path functions
 *
 */

// SDL headers must come first to avoid import→#include bugs on Clang 19.
#include <SDL_filesystem.h>

#include "platform/path.h"

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
	auto* path_base = reinterpret_cast<char8_t *>(SDL_GetBasePath());
	if(!path_base) {
		return {};
	}
	PathData.reset(path_base);
	PathDataView = PathData.get();
	return PathDataView;
}
