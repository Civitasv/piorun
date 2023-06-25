#ifndef PIORUN_COROUTINE_TASK_TERMINATING_H_
#define PIORUN_COROUTINE_TASK_TERMINATING_H_

#include <coroutine>
#include <exception>

#include "coroutine/awaitable/final_continuation.h"

namespace pio {
namespace task {
/**
 * @brief 类似于 chainable，但不可以被 co_await，
 *        可以当作顶层协程返回值使用
 */
struct Terminating {
  struct promise_type {
    Terminating get_return_object() { return Terminating{this}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    awaitable::FinalContinuation final_suspend() noexcept { return {}; }
    void unhandled_exception() { std::terminate(); }
    void return_void() {}

    std::coroutine_handle<> continuation() { return continuation_; }
    void set_continuation(std::coroutine_handle<> continuation) {
      continuation_ = continuation;
    }

    std::coroutine_handle<> continuation_ = std::noop_coroutine();
  };

  using Handle = std::coroutine_handle<promise_type>;

  Terminating(promise_type* p) : coro_handle_(Handle::from_promise(*p)) {}

  Handle coro_handle() { return coro_handle_; }

 private:
  Handle coro_handle_;
};
}  // namespace task
}  // namespace pio

#endif  // PIORUN_COROUTINE_TASK_TERMINATING_H_