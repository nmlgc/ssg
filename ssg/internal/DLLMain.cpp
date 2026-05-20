/*
 *   DLL stub
 *
 */

#if (defined(WIN32) && defined(NDEBUG))

import std.compat;

#pragma comment(linker, "/ENTRY:DllMainCRTStartup")

int __stdcall DllMainCRTStartup(void *, uint32_t, void *)
{
	return 1;
}

#endif
