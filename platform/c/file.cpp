/*
 *   File loading (libc implementation)
 *
 */

#include "platform/file.h"
#include <assert.h>
#include <io.h>

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
	auto size64 = _filelengthi64(fileno(fp));
	if(size64 > size_limit) {
		return {};
	}
	const size_t size = size64;

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

public:
	FILE_STREAM_C(FILE* fp) :
		fp(fp) {
		assert(fp != nullptr);
	}

	~FILE_STREAM_C() {
		fclose(fp);
	}

	bool Seek(int64_t offset, SEEK_WHENCE whence) override {
		int origin;
		switch(whence) {
		case SEEK_WHENCE::BEGIN:  	origin = SEEK_SET;	break;
		case SEEK_WHENCE::CURRENT:	origin = SEEK_CUR;	break;
		case SEEK_WHENCE::END:    	origin = SEEK_END;	break;
		}
		return !_fseeki64(fp, offset, origin);
	};

	std::optional<int64_t> Tell() override {
		auto ret = _ftelli64(fp);
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
	const PATH_LITERAL s, bool fail_if_exists
)
{
	auto* fp = fopen(s, (fail_if_exists ? "wxb" : "wb"));
	if(!fp) {
		return nullptr;
	}
	return std::unique_ptr<FILE_STREAM_C>(new (std::nothrow) FILE_STREAM_C(fp));
}

std::unique_ptr<FILE_STREAM_WRITE> FileStreamWrite(
	const char8_t* s, bool fail_if_exists
)
{
	return FileStreamWrite(
		reinterpret_cast<const PATH_LITERAL>(s), fail_if_exists
	);
}
// -------
