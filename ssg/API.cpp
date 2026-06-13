/*
 *   Public FFI functions
 *
 */

#include "ssg/API.h"
#include "ssg/Replay.h"
#include "ssg/internal/LZ.hpp"
#include "ssg/internal/SSG.hpp"

template <class T> T* LogicFree(T *p)
{
	if(!p) {
		return nullptr;
	}

	// Just `p->~T()` would call `T`'s scalar deleting destructor on MSVC,
	// which contains a branch to deallocate memory via `free()`, which we
	// don't want the linker to pull in.
	p->T::~T();

	BUFFER_HEAP_LOGIC.free(p);
	return nullptr;
}

const std::u8string_view Logic::PackfileBasename(void)
{
	return u8"ENEMY.DAT";
}

const char8_t *LogicPackfileBasename(void)
{
	return Logic::PackfileBasename().data();
}

C_SSG* SSGNew(void)
{
	void *ret = BUFFER_HEAP_LOGIC.allocate(sizeof(C_SSG));
	if(!ret) {
		return nullptr;
	}
	return new (ret) C_SSG;
}

C_SSG* SSG_Free(C_SSG *ssg)
{
	return LogicFree(ssg);
}

PACKFILE_READ* PackfileNew(const void *buf, size_t size)
{
	auto packfile = FilStartR({ static_cast<const uint8_t *>(buf), size });
	if(!packfile) {
		return nullptr;
	}
	auto *ret = static_cast<PACKFILE_READ *>(
		BUFFER_HEAP_LOGIC.allocate(sizeof(PACKFILE_READ))
	);
	if(!ret) {
		return nullptr;
	}
	*ret = packfile;
	return ret;
}

PACKFILE_READ* Packfile_Free(PACKFILE_READ *packfile)
{
	return LogicFree(packfile);
}

const char8_t* ReplayOldBasenameFor(uint8_t stage)
{
	return Replay::OldBasenameFor(stage).data();
}

uint8_t ReplayOldStageNumDetect(const char8_t *fn)
{
	return Replay::OldStageNumDetect(fn);
}
