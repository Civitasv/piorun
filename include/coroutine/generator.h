#ifndef PIORUN_COROUTINE_GENERATOR_H_
#define PIORUN_COROUTINE_GENERATOR_H_

#include <coroutine>
#include <utility>

namespace pio {

template <typename T>
struct Generator {
 public:
  struct promise_type {
   private:
    T val_;

   public:
    Generator get_return_object() { return Generator{this}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }

    std::suspend_always yield_value(T v) {
      val_ = v;
      return {};
    }

    void unhandled_exception() {}

    T val() { return val_; }
  };

  using Handle = std::coroutine_handle<promise_type>;

  explicit Generator(promise_type* p)
      : coro_handle_(Handle::from_promise(*p)) {}

  Generator(Generator&& rhs)
      : coro_handle_(std::exchange(rhs.coro_handle_, nullptr)) {}

  ~Generator() {
    if (coro_handle_) coro_handle_.destroy();
  }

  T Value() const { return coro_handle_.promise().val(); }
  bool Finished() const { return coro_handle_.done(); }
  void Resume() {
    if (not Finished()) {
      coro_handle_.resume();
    }
  }

 private:
  Handle coro_handle_;

 public:
  // for range-loop enhancement
  struct Sentinel {};
  struct Iterator {
    Handle coro_handle_;
    bool operator==(Sentinel) const { return coro_handle_.done(); }

    Iterator& operator++() {
      coro_handle_.resume();
      return *this;
    }

    T operator*() { return coro_handle_.promise().val(); }
    const T operator*() const { return coro_handle_.promise().val(); }
  };

  Iterator begin() { return {coro_handle_}; }
  Sentinel end() { return {}; }
};

}  // namespace pio

#endif  // PIORUN_COROUTINE_GENERATOR_H_