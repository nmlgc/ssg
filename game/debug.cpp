/*                                                                           */
/*   DX_ERROR.c   DirectX のエラー出力用関数                                 */
/*                                                                           */
/*                                                                           */

#include <SDL3/SDL_iostream.h>

#include "game/debug.h"
#include "platform/file.h"
#include "platform/time.h"
#pragma message(PBGWIN_DX_ERROR_H)


// グローバル変数 //
constexpr auto ErrorOut = u8"ErrLOG_UTF8.TXT";
static bool ErrorActive = false;


void DebugLog(std::u8string_view prefix, std::u8string_view s)
{
	if(!ErrorActive) {
		return;
	}
	auto *f = SDL_IOFromFile(ErrorOut, "ab");
	if(!f) {
		return;
	}
	SDL_WriteIO(f, prefix.data(), prefix.size());
	SDL_WriteIO(f, s.data(), s.size());
	SDL_WriteIO(f, "\n", 1);
	SDL_CloseIO(f);
}

extern void DebugSetup()
{
	#pragma warning(suppress : 26494) // type.5
	std::array<char8_t, 64> str;

	const auto tm = Time_NowLocal();
	const auto len = snprintf(
		std::bit_cast<char *>(str.data()),
		str.size(),
		"[%02u/%02u/%02u][%02u:%02u:%02u]",
		tm.month,
		tm.day,
		(tm.year % 100),
		tm.hour,
		tm.minute,
		tm.second
	);
	if(len <= 0) {
		return;
	}
	ErrorActive = true;
	DebugLog(u8"", { str.data(), static_cast<size_t>(len) });
}

extern void DebugCleanup(void)
{
	ErrorActive = false;
}

void DebugLog(std::u8string_view s)
{
	return DebugLog(u8"", s);
}

extern void DebugOut(std::u8string_view s)
{
	return DebugLog(u8"Error : ", s);
}
