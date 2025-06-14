/*
 *   File loading (libc implementation)
 *
 */

#include "platform/file.h"
#include "game/enum_flags.h"
#include <assert.h>
#include <fcntl.h> // For `AT_FDCWD`
#include <stdio.h> // For fileno() and the `SEEK_*` macros
#include <sys/stat.h> // The stat types aren't part of the `std` module either
#include <unistd.h> // For dup() and close()

static size_t LoadInplace(std::span<uint8_t> buf, FILE*&& fp)
{
	if(!fp) {
		return 0;
	}
	auto bytes_read = fread(buf.data(), 1, buf.size_bytes(), fp);
	fclose(fp);
	return bytes_read;
}

static bool WriteAndClose(
	FILE *&&fp, std::span<const BYTE_BUFFER_BORROWED> bufs
)
{
	if(!fp) {
		return false;
	}
	auto ret = [&]() {
		for(const auto& buf : bufs) {
			if(fwrite(buf.data(), buf.size_bytes(), 1, fp) != 1) {
				return false;
			}
		}
		return true;
	}();
	return (!fclose(fp) && ret);
}

size_t FileLoadInplace(std::span<uint8_t> buf, const PATH_LITERAL s)
{
	return LoadInplace(buf, fopen(s, "rb"));
}

size_t FileLoadInplace(std::span<uint8_t> buf, const char8_t* s)
{
	return FileLoadInplace(buf, reinterpret_cast<const PATH_LITERAL>(s));
}

static BYTE_BUFFER_OWNED FPReadAll(
	FILE* fp, size_t size_limit = (std::numeric_limits<size_t>::max)()
)
{
	assert(fp);

	struct stat st;
	if(fstat(fileno(fp), &st) != 0) {
		return {};
	}
	const size_t size = st.st_size;
	if(size > size_limit) {
		return {};
	}

	auto buf = BYTE_BUFFER_OWNED{ size };
	if(!buf) {
		return {};
	}
	if(fread(buf.get(), 1, size, fp) != size) {
		return {};
	}
	return buf;
}

BYTE_BUFFER_OWNED FileLoad(const PATH_LITERAL s, size_t size_limit)
{
	auto fp = fopen(s, "rb");
	if(!fp) {
		return {};
	}
	auto ret = FPReadAll(fp, size_limit);
	fclose(fp);
	return ret;
}

BYTE_BUFFER_OWNED FileLoad(const char8_t* s, size_t size_limit)
{
	return FileLoad(reinterpret_cast<const PATH_LITERAL>(s), size_limit);
}

bool FileWrite(const PATH_LITERAL s, std::span<const BYTE_BUFFER_BORROWED> bufs)
{
	return WriteAndClose(fopen(s, "wb"), bufs);
}

bool FileWrite(const char8_t* s, std::span<const BYTE_BUFFER_BORROWED> bufs)
{
	return FileWrite(reinterpret_cast<const PATH_LITERAL>(s), bufs);
}

bool FileAppend(
	const PATH_LITERAL s, std::span<const BYTE_BUFFER_BORROWED> bufs
)
{
	auto fp = fopen(s, "ab");
	return (
		fp &&
		!fseek(fp, 0, SEEK_END) &&
		WriteAndClose(std::move(fp), bufs)
	);
}

bool FileAppend(const char8_t* s, std::span<const BYTE_BUFFER_BORROWED> bufs)
{
	return FileAppend(reinterpret_cast<const PATH_LITERAL>(s), bufs);
}

// Streams
// -------

struct FILE_STREAM_C : public FILE_STREAM_READ, FILE_STREAM_WRITE {
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

	size_t Read(std::span<uint8_t> buf) override {
		return fread(buf.data(), 1, buf.size_bytes(), fp);
	}

	BYTE_BUFFER_OWNED ReadAll() override {
		if(fseek(fp, 0, SEEK_SET)) {
			return {};
		}
		return FPReadAll(fp);
	}

	bool Write(BYTE_BUFFER_BORROWED buf) override {
		// fwrite() won't return 1 if we try to write a zero-sized buffer.
		if(buf.empty()) {
			return true;
		}
		return (fwrite(buf.data(), buf.size(), 1, fp) == 1);
	};
};

std::unique_ptr<FILE_STREAM_READ> FileStreamRead(const PATH_LITERAL s)
{
	auto* fp = fopen(s, "rb");
	if(!fp) {
		return nullptr;
	}
	return std::unique_ptr<FILE_STREAM_C>(new (std::nothrow) FILE_STREAM_C(fp));
}

std::unique_ptr<FILE_STREAM_READ> FileStreamRead(const char8_t* s)
{
	return FileStreamRead(reinterpret_cast<const PATH_LITERAL>(s));
}

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

	const auto *mode = (!!(flags & FILE_FLAGS::FAIL_IF_EXISTS) ? "wxb" : "wb");
	auto* fp = fopen(s, mode);
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
