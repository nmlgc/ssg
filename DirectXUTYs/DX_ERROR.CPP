/*                                                                           */
/*   DX_ERROR.c   DirectX のエラー出力用関数                                 */
/*                                                                           */
/*                                                                           */

#include "platform/file.h"
#include "DX_ERROR.H"
#include <array>
#include <time.h>
#pragma message(PBGWIN_DX_ERROR_H)


// グローバル変数 //
constexpr auto ErrorOut = _PATH("ErrLOG_UTF8.TXT");
static bool ErrorActive = false;


extern void DebugSetup()
{
	std::array<char, 9> date; // size defined by ABI
	std::array<char, 9> time; // size defined by ABI
	std::array<char, 64> str;

	_strdate(date.data());
	_strtime(time.data());
	auto len = snprintf(
		str.data(), str.size(), "[%s][%s]\n", date.data(), time.data()
	);

	if(len <= 0) {
		return;
	}
	FileAppend(ErrorOut,
		std::span<char>{ str.data(), static_cast<size_t>(len) }
	);
	ErrorActive = true;
}

extern void DebugCleanup(void)
{
	ErrorActive = false;
}

extern void DebugOut(std::string_view s)
{
	if(!ErrorActive) {
		return;
	}
	const std::array<BYTE_BUFFER_BORROWED, 3> bufs = {
		std::string_view{ "Error : " }, s, std::string_view{ "\n" },
	};
	FileAppend(ErrorOut, std::span<const BYTE_BUFFER_BORROWED>{ bufs });
}
