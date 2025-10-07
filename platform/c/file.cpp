/*
 *   File I/O (libc implementation)
 *
 */

// GCC 15 throws `error: redefinition of 'struct timespec'` if this appears
// after a module import.
#include <assert.h>
#include <fcntl.h> // For `AT_FDCWD`
#include <stdio.h> // For fileno() and the `SEEK_*` macros
#include <sys/stat.h> // The stat types aren't part of the `std` module either
#include <unistd.h> // For dup() and close()

#include "platform/file.h"
#include "game/enum_flags.h"

// Streams
// -------

struct FILE_STREAM_C : public FILE_STREAM_WRITE {
	FILE* fp;
	std::optional<statx_timestamp> maybe_mtime;

public:
	FILE_STREAM_C(
		FILE* fp, std::optional<statx_timestamp> maybe_mtime = std::nullopt
	) :
		fp(fp), maybe_mtime(maybe_mtime) {
		assert(fp != nullptr);
	}

	~FILE_STREAM_C() {
		// fclose() overwrites the mtime, so we must change it afterward.
		if(!maybe_mtime) {
			fclose(fp);
			return;
		}
		const auto& mtime = maybe_mtime.value();
		const auto fd = dup(fileno(fp));
		fclose(fp);
		struct timespec times[2] = {
			// Access time. Must be touched in order to change mtime!
			{ .tv_sec = 0, .tv_nsec = UTIME_NOW },

			// Modification time
			{ .tv_sec = mtime.tv_sec, .tv_nsec = mtime.tv_nsec },
		};
		futimens(fd, times);
		close(fd);
	}

	bool Seek(int64_t offset, SEEK_WHENCE whence) override {
		int origin;
		switch(whence) {
		case SEEK_WHENCE::BEGIN:  	origin = SEEK_SET;	break;
		case SEEK_WHENCE::CURRENT:	origin = SEEK_CUR;	break;
		case SEEK_WHENCE::END:    	origin = SEEK_END;	break;
		}
		return !fseeko64(fp, offset, origin);
	};

	std::optional<int64_t> Tell() override {
		auto ret = ftello64(fp);
		if(ret == -1) {
			return std::nullopt;
		}
		return ret;
	};

	bool Write(BYTE_BUFFER_BORROWED buf) override {
		// fwrite() won't return 1 if we try to write a zero-sized buffer.
		if(buf.empty()) {
			return true;
		}
		return (fwrite(buf.data(), buf.size(), 1, fp) == 1);
	};
};

std::unique_ptr<FILE_STREAM_WRITE> FileStreamWrite(
	const PATH_LITERAL s, FILE_FLAGS flags
)
{
	const auto maybe_timestamps = ([&] -> std::optional<statx_timestamp> {
		if(!(flags & FILE_FLAGS::PRESERVE_TIMESTAMPS)) {
			return std::nullopt;
		}
		struct statx stx;
		if(statx(AT_FDCWD, s, 0, STATX_MTIME, &stx) == 0) {
			return stx.stx_mtime;
		}
		struct stat st;
		if(stat(s, &st) == 0) {
			return statx_timestamp{ .tv_sec = st.st_mtime };
		}
		return std::nullopt;
	})();

	auto* fp = fopen(s, "wb");
	if(!fp) {
		return nullptr;
	}
	return std::unique_ptr<FILE_STREAM_C>(
		new (std::nothrow) FILE_STREAM_C(fp, maybe_timestamps)
	);
}

std::unique_ptr<FILE_STREAM_WRITE> FileStreamWrite(
	const char8_t* s, FILE_FLAGS flags
)
{
	return FileStreamWrite(std::bit_cast<const PATH_LITERAL>(s), flags);
}
// -------
