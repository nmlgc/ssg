/*
 *   String formatting helpers
 *
 */

#pragma once

#include <array>
#include <iterator>

// Number of decimal digits required to store the highest value of the given
// type.
template <class T> constexpr auto STRING_NUM_CAP = (
	((241 * sizeof(T)) / 100) + 1
);

namespace String {
	template <size_t Width, class T> struct Num {
		std::array<char, ((std::max)(Width, STRING_NUM_CAP<T>) + 1)> str;
		const size_t len;

		Num(T v) : len(sprintf(str.data(), "%0*u", Width, v)) {
		}
	};
}

template <size_t Width, class T, std::incrementable I> I StringCopyNum(
	T num, I out
) {
	String::Num<Width, T> n = { num };
	out = std::copy_n(n.str.data(), n.len, out);
	*out = '\0';
	return out;
}

template <size_t Width, class T, class S> typename S::iterator StringCopyNum(
	T num, typename S::iterator out
) {
	return StringCopyNum<Width, typename S::iterator, T>(out, num);
}
