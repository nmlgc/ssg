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

REPLAY_OLD* ReplayOldLoad(const void *buf, size_t size)
{
	auto replay = Replay::OldLoad(
		std::span<const uint8_t>{ static_cast<const uint8_t *>(buf), size }
	);
	if(!replay) {
		return nullptr;
	}

	auto *ret = static_cast<REPLAY_OLD *>(
		BUFFER_HEAP_LOGIC.allocate(sizeof(REPLAY_OLD))
	);
	if(!ret) {
		return nullptr;
	}
	ret->Info = std::move(replay.Info);
	ret->Frames = std::move(replay.Frames);
	return ret;
}

REPLAY_OLD_PTRS ReplayOld_Data(const REPLAY_OLD *replay)
{
	if(!replay) {
		return REPLAY_OLD_PTRS{ .Info = nullptr, .Frames = nullptr };
	}
	return REPLAY_OLD_PTRS{
		.Info = replay->Info.get(),
		.Frames = replay->Frames.get(),
	};
}

REPLAY_OLD *ReplayOld_Free(REPLAY_OLD *replay)
{
	return LogicFree(replay);
}
