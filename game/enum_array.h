/*
 *   std::array with one element for each member of an enum, delimited with a
 *   `COUNT` field.
 */

#pragma once

#include <array>
#include <utility>

template <class E> concept ENUMARRAY_ID = (
	std::is_enum_v<E> &&
	std::is_unsigned_v<std::underlying_type_t<E>> &&
	requires { E::COUNT; }
);

template <class T, ENUMARRAY_ID IDType> class ENUMARRAY : public std::array<
	T, std::to_underlying(IDType::COUNT)
> {
	using BASE = std::array<T, std::to_underlying(IDType::COUNT)>;

public:
	constexpr T& operator[](IDType id) noexcept {
		return BASE::operator[](std::to_underlying(id));
	}

	constexpr const T& operator[](IDType id) const noexcept {
		return BASE::operator[](std::to_underlying(id));
	}
};
