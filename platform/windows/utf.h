/*
 *   Unicode conversion helpers
 *
 */

#pragma once

#define WIN32_LEAN_AND_MEAN

#include "game/narrow.h"
#include <assert.h>
#include <malloc.h>
#include <optional>
#include <type_traits>
#include <windows.h>

namespace UTF {

template <typename R> std::optional<R> WithUTF16(
	Narrow::string_view str,
	std::invocable<const std::wstring_view> auto&& func,
	UINT fallback = 932
)
{
	if(str.size() == 0) {
		return std::nullopt;
	}

	auto* str_w = static_cast<wchar_t *>(_malloca(
		str.size() * sizeof(wchar_t)
	));
	if(!str_w) {
		return std::nullopt;
	}

	// Try UTF-8
	auto len_w = MultiByteToWideChar(
		CP_UTF8, MB_ERR_INVALID_CHARS, str.data(), str.size(), str_w, str.size()
	);
	if((len_w == 0) && (GetLastError() == ERROR_NO_UNICODE_TRANSLATION)) {
		if(fallback == 0) {
			return std::nullopt;
		}
		len_w = MultiByteToWideChar(
			fallback, MB_PRECOMPOSED, str.data(), str.size(), str_w, str.size()
		);
		assert(len_w > 0);
	}
	auto ret = func({ str_w, static_cast<size_t>(len_w) });
	_freea(str_w);
	return ret;
}

template <typename R> std::optional<R> WithUTF16(
	std::u8string_view str, std::invocable<const std::wstring_view> auto&& func
)
{
	return WithUTF16<R>(str, func, 0);
}

// Wrapper for null-terminated strings that are guaranteed to be UTF-8.
template <typename R> std::optional<R> WithUTF16(
	const char8_t* str, std::invocable<const std::wstring_view> auto&& func
)
{
	// The terminating \0 must be part of the view.
	const size_t len = (strlen(reinterpret_cast<const char*>(str)) + 1);
	return WithUTF16<R>({ str, len }, func);
}

}
