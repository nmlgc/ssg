/*
 *   File I/O (Win32 implementation)
 *
 */

#define WIN32_LEAN_AND_MEAN

#include <SDL3/SDL_iostream.h>

#include "platform/windows/utf.h"
#include "platform/file.h"
#include "game/defer.h"
#include <windows.h>

struct FILE_TIMESTAMPS_WIN32 : public FILE_TIMESTAMPS {
	FILETIME ctime;
	FILETIME mtime;
};

static std::unique_ptr<FILE_TIMESTAMPS> File_TimestampsGetW(
	const std::wstring_view fn_w
)
{
	auto handle = CreateFileW(
		fn_w.data(),
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN,
		nullptr
	);
	if(handle == INVALID_HANDLE_VALUE) {
		return nullptr;
	}
	defer(CloseHandle(handle));
	FILE_TIMESTAMPS_WIN32 ret;
	if(!GetFileTime(handle, &ret.ctime, nullptr, &ret.mtime)) {
		return nullptr;
	}
	return std::unique_ptr<FILE_TIMESTAMPS_WIN32>(
		new (std::nothrow) FILE_TIMESTAMPS_WIN32(ret)
	);
}

std::unique_ptr<FILE_TIMESTAMPS> File_TimestampsGet(const char8_t *fn)
{
	return UTF::WithUTF16<std::unique_ptr<FILE_TIMESTAMPS>>(
		fn, File_TimestampsGetW
	).value_or(nullptr);
}

bool File_CloseWithTimestamps(
	SDL_IOStream *&& context, std::unique_ptr<FILE_TIMESTAMPS> maybe_timestamps
)
{
	const auto *times = std::bit_cast<FILE_TIMESTAMPS_WIN32 *>(
		maybe_timestamps.get()
	);
	if(times) {
		const SDL_PropertiesID props = SDL_GetIOProperties(context);
		if(props) {
			auto handle = std::bit_cast<HANDLE>(SDL_GetPointerProperty(
				props,
				SDL_PROP_IOSTREAM_WINDOWS_HANDLE_POINTER,
				INVALID_HANDLE_VALUE
			));
			SetFileTime(handle, &times->ctime, nullptr, &times->mtime);
		}
	}
	maybe_timestamps.reset();
	return SDL_CloseIO(context);
}
