#ifndef PIORUN_COROUTINE_AWAITABLE_FINAL_CONTINUATION_H_
#define PIORUN_COROUTINE_AWAITABLE_FINAL_CONTINUATION_H_

#include <coroutine>

#include "utils/concepts.h"

namespace pio {
namespace awaitable {

struct FinalContinuation {
  constexpr bool await_ready() const noexcept { return false; }
  std::coroutine_handle<> await_suspend(
      HandleWithContinuation auto caller) const noexcept {
    // 这里返回在 `task.await_suspend` 中存储的 continuation
    // 目的是将控制权转移到 continuation
    return caller.promise().continuation();
  }
  constexpr void await_resume() const noexcept {}
};

}  // namespace awaitable
}  // namespace pio

#endif  // PIORUN_COROUTINE_AWAITABLE_FINAL_CONTINUATION_H_