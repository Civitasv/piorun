#ifndef PIORUN_COROUTINE_LAZY_H_
#define PIORUN_COROUTINE_LAZY_H_

#include <coroutine>

#include "promise.h"

namespace pio {

template <typename T = void, bool Joinable = false>
class Lazy {
 public:
  using promise_type = Promise<Lazy, T, Joinable>;
  Lazy() noexcept = default;

  Lazy(std::coroutine_handle<promise_type> coroutine) noexcept
      : coroutine_(coroutine) {}

  Lazy(Lazy const&) = delete;
  Lazy& operator=(Lazy const&) = delete;
  Lazy(Lazy&& other) noexcept : coroutine_(other.coroutine_) {
    other.coroutine_ = nullptr;
  }
  Lazy& operator=(Lazy&& other) noexcept {
    if (this != &other) {
      // For joinable tasks, the coroutine is destroyed in the final
      // awaiter to support fire-and-forget semantics
      if constexpr (!Joinable) {
        if (coroutine_) {
          coroutine_.destroy();
        }
      }
      coroutine_ = other.coroutine_;
      other.coroutine_ = nullptr;
    }
    return *this;
  }
  bool Done() const noexcept { return coroutine_.done(); }

  ~Lazy() noexcept {
    if constexpr (!Joinable) {
      if (coroutine_) {
        coroutine_.destroy();
      }
    }
  }

  // The dereferencing operators below return the data contained in the
  // associated promise
  [[nodiscard]] auto operator*() noexcept {
    static_assert(!std::is_same_v<T, void>,
                  "This task doesn't contain any data");
    return std::ref(promise().data_);
  }

  [[nodiscard]] auto operator*() const noexcept {
    static_assert(!std::is_same_v<T, void>,
                  "This task doesn't contain any data");
    return std::cref(promise().data_);
  }

  // A task_t is truthy if it is not associated with an outstanding
  // coroutine or the coroutine it is associated with is complete
  [[nodiscard]] operator bool() const noexcept { return await_ready(); }

  [[nodiscard]] bool await_ready() const noexcept {
    return !coroutine_ || coroutine_.done();
  }

  void Join() {
    static_assert(Joinable,
                  "Cannot join a task without the Joinable type "
                  "parameter "
                  "set");
    coroutine_.promise().join_sem.acquire();
  }

  // When suspending from a coroutine *within* this task's coroutine, save
  // the resume point (to be resumed when the inner coroutine finalizes)
  std::coroutine_handle<> await_suspend(
      std::coroutine_handle<> coroutine) noexcept {
    return pio::await_suspend(coroutine_, coroutine);
  }

  // The return value of await_resume is the final result of `co_await
  // this_task` once the coroutine associated with this task completes
  auto await_resume() const noexcept {
    if constexpr (std::is_same_v<T, void>) {
      return;
    } else {
      return std::move(promise().data_);
    }
  }
  std::coroutine_handle<promise_type> coroutine() { return coroutine_; }

 protected:
  [[nodiscard]] promise_type& promise() const noexcept {
    return coroutine_.promise();
  }

 private:
  std::coroutine_handle<promise_type> coroutine_ = nullptr;
};
}  // namespace pio

#endif  // PIORUN_COROUTINE_LAZY_H_
