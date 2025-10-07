/*
 *   File I/O (Win32 implementation)
 *
 */

#define WIN32_LEAN_AND_MEAN

#include "platform/windows/utf.h"
#include "platform/file.h"
#include "game/defer.h"
#include "game/enum_flags.h"
#include <assert.h>
#include <windows.h>

struct TIMESTAMPS {
	FILETIME ctime;
	FILETIME mtime;
};

static HANDLE OpenRead(const PATH_LITERAL s)
{
	return CreateFileW(
		s,
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN,
		nullptr
	);
}

static HANDLE OpenWrite(const PATH_LITERAL s, DWORD disposition)
{
	// Use `FILE_GENERIC_WRITE` instead of `GENERIC_WRITE` to ensure that we
	// can write attributes.
	return CreateFileW(
		s, FILE_GENERIC_WRITE, 0, nullptr, disposition, 0, nullptr
	);
}

static bool HandleWrite(HANDLE handle, const BYTE_BUFFER_BORROWED buf)
{
	[[gsl::suppress(type.5)]] DWORD bytes_written;
	if((handle == INVALID_HANDLE_VALUE) || !WriteFile(
		handle, buf.data(), buf.size_bytes(), &bytes_written, nullptr
	)) {
		return false;
	}
	return (buf.size_bytes() == bytes_written);
}

// Streams
// -------

struct FILE_STREAM_WIN32 : public FILE_STREAM_WRITE {
	HANDLE handle;
	std::optional<TIMESTAMPS> maybe_timestamps;

public:
	FILE_STREAM_WIN32(
		HANDLE handle,
		std::optional<TIMESTAMPS> maybe_timestamps = std::nullopt
	) : handle(handle), maybe_timestamps(maybe_timestamps) {
		assert(handle != INVALID_HANDLE_VALUE);
	}

	~FILE_STREAM_WIN32() {
		if(const auto& times = maybe_timestamps) {
			SetFileTime(handle, &times->ctime, nullptr, &times->mtime);
		}
		CloseHandle(handle);
	}

	bool Seek(int64_t offset, SEEK_WHENCE whence) override {
		DWORD origin = FILE_BEGIN;
		const LARGE_INTEGER offset_large = { .QuadPart = offset };
		switch(whence) {
		case SEEK_WHENCE::BEGIN:  	origin = FILE_BEGIN;  	break;
		case SEEK_WHENCE::CURRENT:	origin = FILE_CURRENT;	break;
		case SEEK_WHENCE::END:    	origin = FILE_END;    	break;
		}
		return SetFilePointerEx(handle, offset_large, nullptr, origin);
	};

	std::optional<int64_t> Tell() override {
		[[gsl::suppress(type.5)]] LARGE_INTEGER ret;
		if(!SetFilePointerEx(handle, { 0 }, &ret, FILE_CURRENT)) {
			return std::nullopt;
		}
		return ret.QuadPart;
	};

	bool Write(BYTE_BUFFER_BORROWED buf) override {
		return HandleWrite(handle, buf);
	};
};

std::unique_ptr<FILE_STREAM_WRITE> FileStreamWrite(
	const PATH_LITERAL s, FILE_FLAGS flags
)
{
	const auto maybe_timestamps = ([&] -> std::optional<TIMESTAMPS> {
		if(!(flags & FILE_FLAGS::PRESERVE_TIMESTAMPS)) {
			return std::nullopt;
		}
		auto handle = OpenRead(s);
		if(handle == INVALID_HANDLE_VALUE) {
			return std::nullopt;
		}
		defer(CloseHandle(handle));
		TIMESTAMPS ret;
		if(!GetFileTime(handle, &ret.ctime, nullptr, &ret.mtime)) {
			return std::nullopt;
		}
		return ret;
	})();

	auto handle = OpenWrite(s, CREATE_ALWAYS);
	if(handle == INVALID_HANDLE_VALUE) {
		return nullptr;
	}
	return std::unique_ptr<FILE_STREAM_WIN32>(
		new (std::nothrow) FILE_STREAM_WIN32(handle, maybe_timestamps)
	);
}

std::unique_ptr<FILE_STREAM_WRITE> FileStreamWrite(
	const char8_t* s, FILE_FLAGS flags
)
{
	return UTF::WithUTF16<std::unique_ptr<FILE_STREAM_WRITE>>(
		s, [flags](const std::wstring_view str_w) {
			return FileStreamWrite(str_w.data(), flags);
		}
	).value_or(nullptr);
}
// -------
