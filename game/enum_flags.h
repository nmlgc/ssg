/*
 *   Operators for using enums as flags.
 *
 */

#pragma once

#include <type_traits>
#include <utility>

template <typename T> concept ENUMFLAGS = (std::is_enum_v<T> && requires {
	{ T::_HAS_BITFLAG_OPERATORS };
});

template <ENUMFLAGS T> constexpr inline std::underlying_type_t<T> operator~(
	const T& v
) noexcept
{
	return ~std::to_underlying(v);
}

template <ENUMFLAGS T> constexpr inline bool operator&(
	const T& a, const T& b
) noexcept
{
	return (std::to_underlying(a) & std::to_underlying(b));
}

template <ENUMFLAGS T> constexpr inline T operator|(
	const T& a, const T& b
) noexcept
{
	return static_cast<T>(std::to_underlying(a) | std::to_underlying(b));
}

template <ENUMFLAGS T> constexpr inline T& operator|=(T& a, const T& b) noexcept
{
	a = static_cast<T>(std::to_underlying(a) | std::to_underlying(b));
	return a;
}

template <ENUMFLAGS T> constexpr inline T& operator&=(
	T& a, const std::underlying_type_t<T>& b
) noexcept
{
	a = static_cast<T>(std::to_underlying(a) & b);
	return a;
}
