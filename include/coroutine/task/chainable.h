#ifndef PIORUN_COROUTINE_TASK_CHAINABLE_H_
#define PIORUN_COROUTINE_TASK_CHAINABLE_H_

#include <coroutine>
#include <exception>
#include <utility>

#include "coroutine/awaitable/data.h"
#include "coroutine/awaitable/final_continuation.h"
#include "utils/concepts.h"

namespace pio {
namespace task {

/**
 * @brief 实现 Chainable task
 * 该类型 task 可以按线性执行，类似于传统函数执行方式
 * @see test_chainable.cc
 */
struct Chainable {
 public:
  struct promise_type {
   public:
    Chainable get_return_object() { return Chainable{this}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    awaitable::FinalContinuation final_suspend() noexcept { return {}; }
    void unhandled_exception() { std::terminate(); }
    void return_value(awaitable::Result result) { result_ = result; }

    /** Setter and getter */
    std::coroutine_handle<> continuation() { return continuation_; }
    void set_continuation(std::coroutine_handle<> continuation) {
      continuation_ = continuation;
    }
    awaitable::Result result() { return result_; }
    /** ================= */

   private:
    std::coroutine_handle<> continuation_ = std::noop_coroutine();
    awaitable::Result result_;
  };

  using Handle = std::coroutine_handle<promise_type>;

  Chainable(const Chainable&) = delete;
  Chainable& operator=(const Chainable&) = delete;

  explicit Chainable(promise_type* p)
      : coro_handle_(Handle::from_promise(*p)) {}

  Chainable(Chainable&& rhs)
      : coro_handle_(std::exchange(rhs.coro_handle_, nullptr)) {}

  Chainable& operator=(Chainable&& rhs) noexcept {
    if (this == &rhs) return *this;
    if (coro_handle_) coro_handle_.destroy();
    coro_handle_ = std::exchange(rhs.coro_handle_, nullptr);

    return *this;
  }

  ~Chainable() {
    if (coro_handle_) coro_handle_.destroy();
  }

  Handle coro_handle() { return coro_handle_; }

 private:
  Handle coro_handle_;  //< 用于恢复协程的执行

 public:
  enum Type { EPOLL = 0, IOURING, NONE };

 public:
  // Make it awaitable
  bool await_ready() noexcept {
    // we want the caller to suspend.
    return false;
  }
  std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle) {
    coro_handle_.promise().set_continuation(handle);
    return coro_handle_;
  }
  awaitable::Result await_resume() { return coro_handle_.promise().result(); }
};
}  // namespace task
}  // namespace pio

#endif  // PIORUN_COROUTINE_TASK_CHAINABLE_H_
