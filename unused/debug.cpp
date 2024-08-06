/*
 *   Unused debugging code
 *
 */

// In Debug mode, the original code called these macros to output information
// about a subset of ECL and SCL commands. This was broken for several reasons:
//
// • These draw calls occur as part of each frame's movement code and before
//   the game clears the backbuffer. Hence, they wouldn't ever be visible.
//
// • Designating only a single row for all ECL and SCL commands isn't all too
//   useful. GrpPut16() is a blit operation that is only reflected on-screen
//   after the next buffer flip, so you'd only see the last ECL command
//   processed during each frame. Printing these debug strings to some kind of
//   stream would have greatly supported the use case of single-stepping in a
//   debugger.
//
// • Without a draw call to clear the respective line with black pixels before
//   every GrpPut16() call, the glyphs of successive debug strings within a
//   single frame would be blitted on top of each other, resulting in an
//   unreadable  mess.
//
// • Besides, the designated positions on the HUD are already occupied with the
//   "Tama1" and "Tama2" output.
//
// • Also, why are they so weirdly indented?
//
// This fork changes these macros to output to the log file instead, which is
// much more useful in the age of hot-reloading text editors and multi-monitor
// setups.

// ＥＣＬデバッグ用マクロ //
#ifdef _DEBUG
	#define ECL_DEBUG(s,param)				\
	{										\
		char _ECL_Debug[1000];				\
		sprintf(_ECL_Debug,s,param);		\
		GrpPut16(10,10+16*10,_ECL_Debug);	\
	}
#else
	#define ECL_DEBUG(s,param)
#endif

// デバッグ用マクロ //
#ifdef _DEBUG
	#define SCL_DEBUG(s)					\
	{										\
		GrpPut16(10,10+16*11,s);			\
	}
#else
	#define SCL_DEBUG(s)
#endif
