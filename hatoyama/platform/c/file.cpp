/*
 *   File I/O (libc implementation)
 *
 */

#include <SDL3/SDL_iostream.h>

// GCC 15 throws `error: redefinition of 'struct timespec'` if this appears
// after a module import.
#include <fcntl.h> // For `AT_FDCWD`
#include <stdio.h> // For fileno()
#include <sys/stat.h> // The stat types aren't part of the `std` module either
#include <unistd.h> // For dup() and close()

#include "platform/file.h"

struct FILE_TIMESTAMPS_C : public FILE_TIMESTAMPS {
	statx_timestamp mtime;
};

std::unique_ptr<FILE_TIMESTAMPS> File_TimestampsGet(const char8_t *fn)
{
	const auto *s = std::bit_cast<const char *>(fn);

	struct statx stx;
	if(statx(AT_FDCWD, s, 0, STATX_MTIME, &stx) == 0) {
		FILE_TIMESTAMPS_C ret = { .mtime = stx.stx_mtime };
		return std::unique_ptr<FILE_TIMESTAMPS_C>(
			new (std::nothrow) FILE_TIMESTAMPS_C(ret)
		);
	}

	struct stat st;
	if(stat(s, &st) == 0) {
		FILE_TIMESTAMPS_C ret = { .mtime = { .tv_sec = st.st_mtime } };
		return std::unique_ptr<FILE_TIMESTAMPS_C>(
			new (std::nothrow) FILE_TIMESTAMPS_C(ret)
		);
	}

	return nullptr;
}

bool File_CloseWithTimestamps(
	SDL_IOStream *&& context, std::unique_ptr<FILE_TIMESTAMPS> maybe_timestamps
)
{
	const auto *times = std::bit_cast<FILE_TIMESTAMPS_C *>(
		maybe_timestamps.get()
	);
	// fclose() overwrites the mtime, so we must change it afterward.
	if(!times) {
		return SDL_CloseIO(context);
	}

	const SDL_PropertiesID props = SDL_GetIOProperties(context);
	if(!props) {
		return SDL_CloseIO(context);
	}
	const int fd1 = SDL_GetNumberProperty(
		props, SDL_PROP_IOSTREAM_FILE_DESCRIPTOR_NUMBER, -1
	);
	if(fd1 == -1) {
		return SDL_CloseIO(context);
	}

	const auto fd = dup(fd1);
	const bool ret = SDL_CloseIO(context);

	struct timespec ts[2] = {
		// Access time. Must be touched in order to change mtime!
		{ .tv_sec = 0, .tv_nsec = UTIME_NOW },

		// Modification time
		{ .tv_sec = times->mtime.tv_sec, .tv_nsec = times->mtime.tv_nsec },
	};
	futimens(fd, ts);
	close(fd);
	maybe_timestamps.reset();
	return ret;
}
