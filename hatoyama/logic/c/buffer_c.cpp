#include "logic/buffer.h"

namespace {

void *Malloc(size_t n) noexcept
{
	return malloc(n);
}

void Free(void *p) noexcept
{
	free(p);
}

}

const BUFFER_HEAP BUFFER_HEAP_LOGIC = {
	.allocate = Malloc,
	.free = Free,
};