/*
 *   std::array with one element for each member of an enum, delimited with a
 *   `COUNT` field.
 */

#pragma once

#include <array>
#include <assert.h>
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
	// We could do something like
	//
	//	using T_ref_or_value = std::conditional_t<
	//		std::derived_from<T, std::string_view>, T, T&
	//	>;
	//
	// to opt into returning by value for certain classes, but this does not
	// compile on Visual Studio 2022 17.11.0 Preview 4.0.

	constexpr T& operator[](IDType id) noexcept {
		[[gsl::suppress(gsl.view)]]
		return BASE::operator[](std::to_underlying(id));
	}

	constexpr const T& operator[](IDType id) const noexcept {
		[[gsl::suppress(gsl.view)]]
		return BASE::operator[](std::to_underlying(id));
	}
};

namespace Cast {

template <ENUMARRAY_ID IDType> constexpr inline IDType down_enum(
	std::underlying_type_t<IDType> f
)
{
	assert(f < std::to_underlying(IDType::COUNT));
	return static_cast<IDType>(f);
}

} // namespace Cast
