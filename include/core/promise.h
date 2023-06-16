#ifndef PIORUN_CORE_PROMISE_H_
#define PIORUN_CORE_PROMISE_H_

#include <coroutine>
#include <exception>

#include "lazy.h"

namespace pio {
template <typename T>
class PromiseBase {
 public:
  PromiseBase() = default;
  virtual ~PromiseBase() = default;
};

template <typename T>
class Promise : public PromiseBase<T> {
 public:
  Promise() = default;
  std::suspend_never initial_suspend() const noexcept { return {}; }
  std::suspend_never final_suspend() const noexcept { return {}; }
  void unhandled_exception() { std::terminate(); }
  Lazy<T> get_return_object() {
    return Lazy<T>(std::coroutine_handle<Promise<T>>::from_promise(*this));
  }
  T return_value(T&& value) noexcept {
    value_ = std::forward<T>(value);
    return value_;
  }
  T get_return_value() const { return value_; }

 private:
  T value_;
};

template <>
class Promise<void> : public PromiseBase<void> {
 public:
  Promise() = default;
  std::suspend_never initial_suspend() const noexcept { return {}; }
  std::suspend_never final_suspend() const noexcept { return {}; }
  void unhandled_exception() { std::terminate(); }
  Lazy<void> get_return_object() {
    return Lazy<void>(
        std::coroutine_handle<Promise<void>>::from_promise(*this));
  }
  void return_void() noexcept {}
  void get_return_value() const {}
};

}  // namespace pio

#endif