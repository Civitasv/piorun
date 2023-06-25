#ifndef PIORUN_COROUTINE_AWAITABLE_UNIVERSAL_H_
#define PIORUN_COROUTINE_AWAITABLE_UNIVERSAL_H_

#include <coroutine>

#include "coroutine/awaitable/event.h"
#include "utils/concepts.h"

namespace pio {
namespace awaitable {

void NotifyEmitters(Event *);

struct Universal {
  Event event_;
  std::coroutine_handle<> handle_;

  bool await_ready() {
    if (!event_.condition) return false;
    bool result = event_.condition();
    if (result) event_.result.result_type = EventType::WAKEUP;
    return result;
  }

  std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) {
    event_.continuation = caller;
    NotifyEmitters(&event_); ///< 事件来了，需要重新注册
    return handle_;
  }

  Result await_resume() { return event_.result; }
};

}  // namespace awaitable
}  // namespace pio

#endif  // PIORUN_COROUTINE_AWAITABLE_UNIVERSAL_H_