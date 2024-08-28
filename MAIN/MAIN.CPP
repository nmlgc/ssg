/*
 *   Win32 entry point
 *
 */

#include "../GIAN07/ENTRY.H"
#include "game/defer.h"
#include "platform/window_backend.h"

int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
	if(!WndBackend_Win32Create()) {
		return FALSE;
	}
	defer(WndBackend_Cleanup());

	if(!XInit()) {
		return false;
	}
	defer(XCleanup());

	return WndBackend_Run();
}
