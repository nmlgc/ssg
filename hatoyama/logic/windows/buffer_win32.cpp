/*
 *   Memory ownership semantics for Win32 `HeapAlloc()` buffers
 *
 */

#include "logic/buffer.h"
#include <windows.h>

const BUFFER_HEAP BUFFER_HEAP_LOGIC = {
	.allocate = [](size_t size) noexcept {
		return HeapAlloc(GetProcessHeap(), 0, size);
	},
	.free = [](void *buf) noexcept {
		HeapFree(GetProcessHeap(), 0, buf);
	},
};
