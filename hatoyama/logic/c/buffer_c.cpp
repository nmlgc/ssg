/*
 *   Memory ownership semantics for raw C buffers
 *
 */

#include "logic/buffer.h"

const BUFFER_HEAP BUFFER_HEAP_LOGIC = {
	.allocate = [](size_t n) noexcept {
		return malloc(n);
	},
	.free = [](void *p) noexcept {
		return free(p);
	},
};
