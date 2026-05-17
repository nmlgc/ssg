/*
 *   Directional casts for integers
 *
 *   Inspired by the Guidelines Support Library:
 *
 *   	https://github.com/microsoft/GSL
 */

#pragma once

import std;

namespace Cast {

template <class T, class F> constexpr T down(F&& f) noexcept
{
	using To = std::remove_reference_t<T>;
	using From = std::remove_reference_t<F>;
	static_assert(sizeof(To) < sizeof(From));
	static_assert(std::is_signed_v<To> == std::is_signed_v<From>);
	return static_cast<T>(std::forward<F>(f));
}

template <class T, class F> constexpr T sign(F&& f) noexcept
{
	using To = std::remove_reference_t<T>;
	using From = std::remove_reference_t<F>;
	static_assert(sizeof(To) == sizeof(From));
	static_assert(std::is_signed_v<To> != std::is_signed_v<From>);
	return static_cast<T>(std::forward<F>(f));
}

template <class T, class F> constexpr T down_sign(F&& f) noexcept
{
	using To = std::remove_reference_t<T>;
	using From = std::remove_reference_t<F>;
	static_assert(sizeof(To) < sizeof(From));
	static_assert(std::is_signed_v<To> != std::is_signed_v<From>);
	return static_cast<T>(std::forward<F>(f));
}

template <class T, class F> constexpr T up(F&& f) noexcept
{
	using To = std::remove_reference_t<T>;
	using From = std::remove_reference_t<F>;
	static_assert(sizeof(To) > sizeof(From));
	static_assert(std::is_signed_v<To> == std::is_signed_v<From>);
	return static_cast<T>(std::forward<F>(f));
}

template <class T, class F> constexpr T up_sign(F&& f) noexcept
{
	using To = std::remove_reference_t<T>;
	using From = std::remove_reference_t<F>;
	static_assert(sizeof(To) > sizeof(From));
	static_assert(std::is_signed_v<To> != std::is_signed_v<From>);
	return static_cast<T>(std::forward<F>(f));
}

} // namespace Cast
