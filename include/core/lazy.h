#ifndef PIORUN_CORE_LAZY_H_
#define PIORUN_CORE_LAZY_H_

#include <coroutine>

namespace pio {

template <typename T>
class Promise;

template <typename T>
class LazyAwaiter {
 public:
  LazyAwaiter(std::coroutine_handle<Promise<T>> handle) : handle_(handle) {}

  bool await_ready() const noexcept { return handle_.done(); }

  template <typename ContinuationPromiseType>
  std::coroutine_handle<> await_suspend(
      std::coroutine_handle<ContinuationPromiseType> handle) const noexcept {
    handle_.promise().set_continuation(handle);
    return handle_;
  }

  T await_resume() const { return handle_.promise().get_return_value(); }

 private:
  std::coroutine_handle<Promise<T>> handle_;
};

template <typename T>
struct Lazy {
 public:
  using promise_type = Promise<T>;

  Lazy(std::coroutine_handle<promise_type> handle) : handle_(handle) {}

  Lazy(Lazy&) = delete;
  Lazy(Lazy&& other) noexcept : handle_(other.handle_) { other.handle_ = {}; }
  bool Done() const noexcept { return handle_.done(); }

  LazyAwaiter<T> operator co_await() noexcept {
    return LazyAwaiter<T>{handle_};
  }

  ~Lazy() {
    if (handle_) {
      handle_.destroy();
    }
  }

  std::coroutine_handle<promise_type> handle_;
};

}  // namespace pio

#endif