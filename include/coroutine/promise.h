#ifndef PIORUN_COROUTINE_PROMISE_H_
#define PIORUN_COROUTINE_PROMISE_H_

#include <coroutine>
#include <exception>

#include "core/log.h"
#include "core/mutex.h"
#include "core/thread.h"

namespace pio {
auto static logger_coroutine = pio::Logger::Create("piorun_coroutine.log");
template <typename Promise, bool Joinable>
struct FinalAwaiter {
  bool await_ready() const noexcept { return false; }

  void await_resume() const noexcept {}

  std::coroutine_handle<> await_suspend(
      std::coroutine_handle<Promise> coroutine) const noexcept {
    // Check if this coroutine is being finalized from the
    // middle of a "continuation" coroutine and hop back there to
    // continue execution while *this* coroutine is suspended.

    logger_coroutine->Info("Final await for coroutine %p", coroutine.address());
    // After acquiring the flag, the other thread's write to the
    // coroutine's continuation must be visible (one-way
    // communication)
    if (coroutine.promise().flag_.exchange(true, std::memory_order_acquire)) {
      // We're not the first to reach here, meaning the
      // continuation is installed properly (if any)
      auto continuation = coroutine.promise().continuation;
      if (continuation) {
        logger_coroutine->Info("Resuming continuation %p on %p",
                               continuation.address(), coroutine.address());
        return continuation;
      } else {
        logger_coroutine->Error("Coroutine %p missing continuation\n",
                                coroutine.address());
      }
    }
    return std::noop_coroutine();
  }
};

template <typename Promise>
struct FinalAwaiter<Promise, true> {
  bool await_ready() const noexcept { return false; }

  void await_resume() const noexcept {}

  void await_suspend(std::coroutine_handle<Promise> coroutine) const noexcept {
    coroutine.promise().join_sem_.release();
    coroutine.destroy();
  }
};

// Helper function for awaiting on a task. The next resume point is
// installed as a continuation of the task being awaited.
template <typename Promise>
std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> base,
                                      std::coroutine_handle<> next) {
  if constexpr (Promise::joinable_) {
    // Joinable tasks are never awaited and so cannot have a
    // continuation by definition
    return std::noop_coroutine();
  } else {
    logger_coroutine->Info("Installing continuation %p for %p", next.address(),
                           base.address());
    base.promise().continuation = next;
    // The write to the continuation must be visible to a person that
    // acquires the flag
    if (base.promise().flag_.exchange(true, std::memory_order_release)) {
      // We're not the first to reach here, meaning the continuation
      // won't get read
      return next;
    }
    return std::noop_coroutine();
  }
}

// All promises need the `continuation` member, which is set when a
// coroutine is suspended within another coroutine. The `continuation`
// handle is used to hop back from that suspension point when the inner
// coroutine finishes.
template <bool Joinable>
struct PromiseBase {
  constexpr static bool joinable_ = Joinable;
  std::atomic<bool> flag_ = false;
  // When a coroutine suspends, the continuation stores the handle to the
  // resume point, which immediately following the suspend point.
  std::coroutine_handle<> continuation = nullptr;
  // Do not suspend immediately on entry of a coroutine
  std::suspend_never initial_suspend() const noexcept { return {}; }
  void unhandled_exception() const noexcept {
    // piorun doesn't currently handle exceptions.
  }
};

// Joinable tasks need an additional semaphore the joiner can wait on
template <>
struct PromiseBase<true> : public PromiseBase<false> {
  Semaphore join_sem_{0};
};

template <typename Lazy, typename T, bool Joinable>
struct Promise : public PromiseBase<Joinable> {
  T data_;

  Lazy get_return_object() noexcept {
    // On coroutine entry, we store as the continuation a handle
    // corresponding to the next sequence point from the caller.
    return {std::coroutine_handle<Promise>::from_promise(*this)};
  }

  void return_value(T const& value) noexcept(
      std::is_nothrow_copy_assignable_v<T>) {
    data_ = value;
  }

  void return_value(T&& value) noexcept(std::is_nothrow_move_assignable_v<T>) {
    data_ = std::move(value);
  }

  FinalAwaiter<Promise, Joinable> final_suspend() noexcept { return {}; }
};

template <typename Lazy, bool Joinable>
struct Promise<Lazy, void, Joinable> : public PromiseBase<Joinable> {
  Lazy get_return_object() noexcept {
    // On coroutine entry, we store as the continuation a handle
    // corresponding to the next sequence point from the caller.
    return {std::coroutine_handle<Promise>::from_promise(*this)};
  }

  void return_void() noexcept {}

  FinalAwaiter<Promise, Joinable> final_suspend() noexcept { return {}; }
};

}  // namespace pio

#endif  // PIORUN_COROUTINE_PROMISE_H_
