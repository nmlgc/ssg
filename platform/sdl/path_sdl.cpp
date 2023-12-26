/*
 *   Common paths as determined by SDL's path functions
 *
 */

#include "platform/path.h"
#include <memory>
#include <SDL_filesystem.h>

static std::unique_ptr<char8_t[], decltype(&SDL_free)> PathData = {
	nullptr, SDL_free
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
	PathData = decltype(PathData){ path_base, SDL_free };
	PathDataView = PathData.get();
	return PathDataView;
}
