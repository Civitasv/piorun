#ifndef PIORUN_COROUTINE_TASK_H_
#define PIORUN_COROUTINE_TASK_H_

#include <coroutine>
#include <utility>

namespace pio {

class Task {
 public:
  struct promise_type {
   public:
    Task get_return_object() { return Task{this}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
    void return_value(int ret) { ret_ = ret; }

   private:
    int ret_;
    friend Task;
  };

  using Handle = std::coroutine_handle<promise_type>;

  Task(const Task&) = delete;
  Task& operator=(const Task&) = delete;

  explicit Task(promise_type* p)
      : coro_handle_(Handle::from_promise(*p)), finished_(false) {}

  Task(Task&& rhs)
      : coro_handle_(std::exchange(rhs.coro_handle_, nullptr)),
        finished_(rhs.finished_) {}

  Task& operator=(Task&& rhs) noexcept {
    if (this == &rhs) return *this;
    if (coro_handle_) coro_handle_.destroy();
    coro_handle_ = std::exchange(rhs.coro_handle_, nullptr);

    return *this;
  }

  ~Task() {
    if (coro_handle_) coro_handle_.destroy();
  }

  bool Finished() { return finished_; }
  void Resume() {
    if (not Finished()) {
      coro_handle_.resume();
    }
  }

  operator int() { return coro_handle_.promise().ret_; }

 private:
  Handle coro_handle_;  //< 用于恢复协程的执行
  bool finished_;

 public:
  enum Type { EPOLL = 0, IOURING, NONE };

 public:
  // Make it awaitable
  bool await_ready() { return false; }
  void await_suspend(Handle handle) {}
  void await_resume() {}
};
}  // namespace pio

#endif  // PIORUN_COROUTINE_TASK_H_
