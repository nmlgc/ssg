/*
 *   Threads via std::jthread
 *
 */

#pragma once

import std;

struct THREAD_STOP : public std::stop_token {
	explicit operator bool() const {
		return stop_requested();
	}
};

struct THREAD : public std::jthread {
	using std::jthread::jthread;

	bool Joinable() const noexcept {
		return joinable();
	}
	void Join() {
		join();
	}

	THREAD() noexcept = default;
	THREAD(THREAD&& other) noexcept = default;
	THREAD(const THREAD&) = delete;
	THREAD& operator=(THREAD&& other) noexcept = default;
	~THREAD() {
		request_stop();
	}
};

[[nodiscard]] THREAD ThreadStart(std::invocable<const THREAD_STOP&> auto&& func)
{
	return THREAD{ [func = std::move(func)](std::stop_token st) mutable {
		func(THREAD_STOP{ st });
	} };
}
