#ifndef PIORUN_COROUTINE_TASK_MAIN_H_
#define PIORUN_COROUTINE_TASK_MAIN_H_

#include <coroutine>
#include <exception>

namespace pio {
namespace task {
struct Main {
  struct promise_type {
    Main get_return_object() { return Main{this}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() { std::terminate(); }
    void return_value(int ret) { ret_ = ret; }

   private:
    int ret_;
    friend Main;
  };

  using Handle = std::coroutine_handle<promise_type>;
  Main(promise_type* p) : coro_handle_(Handle::from_promise(*p)) {}
  ~Main() {
    if (coro_handle_) coro_handle_.destroy();
  }

  operator int() { return coro_handle_.promise().ret_; }

 private:
  Handle coro_handle_;
};
}  // namespace task
}  // namespace pio

#endif  // PIORUN_COROUTINE_TASK_MAIN_H_