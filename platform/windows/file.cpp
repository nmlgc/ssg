/*
 *   File loading (Win32 implementation)
 *
 */

#define WIN32_LEAN_AND_MEAN

#include "platform/windows/utf.h"
#include "platform/file.h"
#include "game/enum_flags.h"
#include <assert.h>
#include <windows.h>

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
	return CreateFileW(s, GENERIC_WRITE, 0, nullptr, disposition, 0, nullptr);
}

static size_t HandleRead(std::span<uint8_t> buf, HANDLE handle)
{
	[[gsl::suppress(type.5)]] DWORD bytes_read;
	if((handle == INVALID_HANDLE_VALUE) || !ReadFile(
		handle, buf.data(), buf.size_bytes(), &bytes_read, nullptr
	)) {
		return 0;
	}
	return bytes_read;
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

static size_t LoadInplace(std::span<uint8_t> buf, HANDLE&& handle)
{
	const auto ret = HandleRead(buf, handle);
	CloseHandle(handle);
	return ret;
}

static bool WriteAndClose(
	HANDLE&& handle, std::span<const BYTE_BUFFER_BORROWED> bufs
)
{
	const auto ret = [&]() {
		for(const auto& buf : bufs) {
			if(!HandleWrite(handle, buf)) {
				return false;
			}
		}
		return true;
	}();
	return (CloseHandle(handle) && ret);
}

size_t FileLoadInplace(std::span<uint8_t> buf, const PATH_LITERAL s)
{
	return LoadInplace(buf, OpenRead(s));
}

size_t FileLoadInplace(std::span<uint8_t> buf, const char8_t* s)
{
	return UTF::WithUTF16<size_t>(s, [&](const std::wstring_view str_w) {
		return FileLoadInplace(buf, str_w.data());
	}).value_or(0);
}

static BYTE_BUFFER_OWNED HandleReadAll(
	HANDLE handle, size_t size_limit = (std::numeric_limits<size_t>::max)()
)
{
	LARGE_INTEGER size64;
	if(!GetFileSizeEx(handle, &size64) || (size64.QuadPart > size_limit)) {
		return {};
	}
	const size_t size = size64.QuadPart;

	auto buf = BYTE_BUFFER_OWNED{ size };
	if(!buf) {
		return {};
	}
	if(HandleRead({ buf.get(), size }, handle) != size) {
		return {};
	}
	return buf;
}

BYTE_BUFFER_OWNED FileLoad(const PATH_LITERAL s, size_t size_limit)
{
	auto handle = OpenRead(s);
	if(handle == INVALID_HANDLE_VALUE) {
		return {};
	}
	auto ret = HandleReadAll(handle, size_limit);
	CloseHandle(handle);
	return ret;
}

BYTE_BUFFER_OWNED FileLoad(const char8_t* s, size_t size_limit)
{
	return UTF::WithUTF16<BYTE_BUFFER_OWNED>(
		s, [&](const std::wstring_view str_w) {
			return FileLoad(str_w.data(), size_limit);
		}
	).value_or(nullptr);
}

bool FileWrite(const PATH_LITERAL s, std::span<const BYTE_BUFFER_BORROWED> bufs)
{
	return WriteAndClose(OpenWrite(s, CREATE_ALWAYS), bufs);
}

bool FileWrite(const char8_t* s, std::span<const BYTE_BUFFER_BORROWED> bufs)
{
	return UTF::WithUTF16<bool>(s, [&](const std::wstring_view str_w) {
		return FileWrite(str_w.data(), bufs);
	}).value_or(false);
}

bool FileAppend(
	const PATH_LITERAL s, std::span<const BYTE_BUFFER_BORROWED> bufs
)
{
	auto handle = OpenWrite(s, OPEN_ALWAYS);
	return (
		handle &&
		(SetFilePointer(handle, 0, 0, FILE_END) != INVALID_SET_FILE_POINTER) &&
		WriteAndClose(std::move(handle), bufs)
	);
}

bool FileAppend(const char8_t* s, std::span<const BYTE_BUFFER_BORROWED> bufs)
{
	return UTF::WithUTF16<bool>(s, [&](const std::wstring_view str_w) {
		return FileAppend(str_w.data(), bufs);
	}).value_or(false);
}

// Streams
// -------

struct FILE_STREAM_WIN32 : public FILE_STREAM_READ, FILE_STREAM_WRITE {
	HANDLE handle;

public:
	FILE_STREAM_WIN32(HANDLE handle) :
		handle(handle) {
		assert(handle != INVALID_HANDLE_VALUE);
	}

	~FILE_STREAM_WIN32() {
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

	size_t Read(std::span<uint8_t> buf) override {
		return HandleRead(buf, handle);
	}

	BYTE_BUFFER_OWNED ReadAll() override {
		if(SetFilePointer(handle, 0, nullptr, FILE_BEGIN) != 0) {
			return {};
		}
		return HandleReadAll(handle);
	}

	bool Write(BYTE_BUFFER_BORROWED buf) override {
		return HandleWrite(handle, buf);
	};
};

std::unique_ptr<FILE_STREAM_READ> FileStreamRead(const PATH_LITERAL s)
{
	auto handle = OpenRead(s);
	if(handle == INVALID_HANDLE_VALUE) {
		return nullptr;
	}
	return std::unique_ptr<FILE_STREAM_WIN32>(
		new (std::nothrow) FILE_STREAM_WIN32(handle)
	);
}

std::unique_ptr<FILE_STREAM_READ> FileStreamRead(const char8_t* s)
{
	return UTF::WithUTF16<std::unique_ptr<FILE_STREAM_READ>>(
		s, [](const std::wstring_view str_w) {
			return FileStreamRead(str_w.data());
		}
	).value_or(nullptr);
}

std::unique_ptr<FILE_STREAM_WRITE> FileStreamWrite(
	const PATH_LITERAL s, FILE_FLAGS flags
)
{
	const auto disposition = (!!(flags & FILE_FLAGS::FAIL_IF_EXISTS)
		? CREATE_NEW
		: CREATE_ALWAYS
	);
	auto handle = OpenWrite(s, disposition);
	if(handle == INVALID_HANDLE_VALUE) {
		return nullptr;
	}
	return std::unique_ptr<FILE_STREAM_WIN32>(
		new (std::nothrow) FILE_STREAM_WIN32(handle)
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
