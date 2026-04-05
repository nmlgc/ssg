/*
 *   Memory ownership semantics for raw C buffers
 *
 */

#include "logic/buffer.h"

const BUFFER_HEAP BUFFER_HEAP_LOGIC = {
	.allocate = malloc,
	.free = free,
};
