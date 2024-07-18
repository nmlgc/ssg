/*                                                                           */
/*   DX_ERROR.c   DirectX のエラー出力用関数                                 */
/*                                                                           */
/*                                                                           */

#include "game/debug.h"
#include "platform/file.h"
#include "platform/time.h"
#pragma message(PBGWIN_DX_ERROR_H)


// グローバル変数 //
constexpr auto ErrorOut = _PATH("ErrLOG_UTF8.TXT");
static bool ErrorActive = false;


extern void DebugSetup()
{
	[[gsl::suppress(type.5)]] std::array<char, 64> str;
	const auto tm = Time_NowLocal();
	const auto len = snprintf(
		str.data(),
		str.size(),
		"[%02u/%02u/%02u][%02u:%02u:%02u]\n",
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
	FileAppend(ErrorOut, std::span(str.data(), static_cast<size_t>(len)));
	ErrorActive = true;
}

extern void DebugCleanup(void)
{
	ErrorActive = false;
}

extern void DebugOut(std::u8string_view s)
{
	using namespace std::string_view_literals;

	if(!ErrorActive) {
		return;
	}
	const std::array<BYTE_BUFFER_BORROWED, 3> bufs = {
		std::span("Error : "sv), std::span(s), std::span("\n"sv),
	};
	FileAppend(ErrorOut, std::span<const BYTE_BUFFER_BORROWED>{ bufs });
}
