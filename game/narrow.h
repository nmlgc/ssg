/*
 *   Narrow string types
 *
 *   Strings in these types are either in the game's original narrow encoding
 *   or in UTF-8. Rendering functions must handle both possibilities.
 */

#pragma once

#include <string>

namespace Any {
	// Strings from outside the game with an unknown and not yet validated
	// encoding.
	using string_view = std::string_view;
}

namespace Narrow {

class string_view;

struct literal {
	const char* ptr;

	constexpr literal() noexcept = delete;
	constexpr literal(const char* other) noexcept : ptr(other) {
	}

	// Encoding-erasing conversions from UTF-8 are OK.
	literal(const std::u8string& other) noexcept :
		ptr(reinterpret_cast<const char *>(other.c_str())) {
	}
};

class string : public std::string {
	using std::string::string;

public:
	// Concatenating another narrow string might lead to mixed encodings within
	// the same string.
	constexpr string operator+(const string& other) = delete;

	inline constexpr string& operator=(const string_view view);
};

class string_view : public std::string_view {
	using std::string_view::string_view;

public:
	constexpr string_view(const string& str) noexcept :
		std::string_view(str) {
	}

	// This asserts that [other]'s encoding is either of the two.
	constexpr string_view(Any::string_view other) noexcept :
		std::string_view(other) {
	}

	constexpr string_view(const literal& other) noexcept :
		std::string_view(other.ptr) {
	}

	// Encoding-erasing conversions from UTF-8 are OK...
	constexpr string_view(const std::u8string_view other) noexcept :
		std::string_view(
			{ reinterpret_cast<const char*>(other.data()), other.size() }
		) {
	}
	constexpr string_view(const std::u8string& other) noexcept :
		string_view(std::u8string_view{ other }) {
	}

	// ...but not the other way round.
	operator std::u8string_view() const = delete;
	operator std::u8string() const = delete;
};

inline constexpr string& string::operator=(const string_view view) {
	assign(view);
	return *this;
}

}
