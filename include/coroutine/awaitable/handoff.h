#ifndef PIORUN_COROUTINE_AWAITABLE_HANDOFF_H_
#define PIORUN_COROUTINE_AWAITABLE_HANDOFF_H_

#include <coroutine>

#include "coroutine/awaitable/event.h"
#include "utils/concepts.h"

namespace pio {
namespace awaitable {

/**
 * @brief 将控制权交给传入的 handle
 */
struct Handoff {
  Handoff(std::coroutine_handle<> handle) : handle_(handle) {}
  constexpr bool await_ready() const noexcept { return false; }
  std::coroutine_handle<> await_suspend(std::coroutine_handle<>) noexcept {
    return handle_;
  }
  constexpr void await_resume() const noexcept {}

 private:
  std::coroutine_handle<> handle_;
};

}  // namespace awaitable
}  // namespace pio

#endif  // PIORUN_COROUTINE_AWAITABLE_HANDOFF_H_