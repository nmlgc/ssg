/*
 *   Cross-platform input declarations
 *
 */

#include "game/input.h"

// グローバル変数(Public)の実体
INPUT_BITS Key_Data = 0;
INPUT_BITS Pad_Data = 0;
INPUT_SYSTEM_BITS SystemKey_Data = 0;

// MSVC's static analyzer suggests to make the functions below `constexpr`,
// which won't work because they are used in other translation units and this
// is not a header.
#pragma warning(suppress: 26497) // f.4
bool Input_IsOK(INPUT_BITS key)
{
	return ((key == KEY_RETURN) || (key == KEY_TAMA));
}

#pragma warning(suppress: 26497) // f.4
bool Input_IsCancel(INPUT_BITS key)
{
	return ((key == KEY_BOMB) || (key == KEY_ESC));
}

int_fast8_t Input_OptionKeyDelta(INPUT_BITS key)
{
	return (
		(Input_IsOK(key) || (key == KEY_RIGHT)) ? 1 :
		(key == KEY_LEFT) ? -1 :
		0
	);
}
