/*
 *   Threads via SDL
 *
 */

#pragma once

import std;
struct SDL_Thread;

using THREAD_STOP = std::atomic<bool>;

template <std::invocable<const THREAD_STOP&> F> struct THREAD_META {
	F f;
	THREAD_STOP& st;
};

class THREAD {
	SDL_Thread *sdl_thread = nullptr;
	std::unique_ptr<THREAD_STOP> st;

public:
	bool Joinable() const noexcept;
	void Join() noexcept;

	THREAD() noexcept = default;
	THREAD(SDL_Thread *sdl_thread, std::unique_ptr<THREAD_STOP> st);
	THREAD(THREAD&& other) noexcept;
	THREAD(const THREAD&) = delete;
	THREAD& operator=(THREAD&& other) noexcept;
	~THREAD();
};

SDL_Thread *HelpCreateThread(int(*fn)(void *), void *data);

template <std::invocable<const THREAD_STOP&> F> int ThreadFunc(void *data)
{
	// Wrap the leaked functor into a `unique_ptr` to destroy both `F` and
	// [meta] upon returning.
	const std::unique_ptr<THREAD_META<F>> meta(
		static_cast<THREAD_META<F> *>(data)
	);

	meta.get()->f(meta.get()->st);
	return 0;
}

template <
	std::invocable<const THREAD_STOP&> F
> [[nodiscard]] THREAD ThreadStart(F&& f)
{
	// Note the two distinct lifetimes here:
	//
	// • [st] must live as long as the returned `THREAD`. It must also be at a
	//   fixed location in memory since we pass it by pointer to ThreadFunc(),
	//   and this pointer must be preserved across assignment operations.
	//
	// • [meta] must destroy itself at the end of [f] to run `F`'s destructor.
	//   It must also be a pointer because SDL_CreateThread() only takes a
	//   single parameter and we must pass two fields that can't be on the
	//   stack, as this function might have long returned by the time the new
	//   thread starts executing.
	auto st = std::make_unique<THREAD_STOP>();
	auto meta = std::make_unique<THREAD_META<F>>(std::forward<F>(f), *st.get());
	auto sdl_thread = HelpCreateThread(ThreadFunc<F>, meta.get());
	if(!sdl_thread) {
		return {};
	}
	// Ownership is now managed on the thread
	meta.release();
	return THREAD{ sdl_thread, std::move(st) };
}
