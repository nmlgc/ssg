/*
 *   Public FFI functions
 *
 */

#include "ssg/API.h"
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
