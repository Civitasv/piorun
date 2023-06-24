#ifndef PIORUN_COROUTINE_AWAITABLE_UNIVERSAL_H_
#define PIORUN_COROUTINE_AWAITABLE_UNIVERSAL_H_

#include <coroutine>

#include "coroutine/awaitable/data.h"
#include "utils/concepts.h"

namespace pio {
namespace awaitable {

void NotifyEmitters(Data *);

struct Universal {
  Data data_;
  std::coroutine_handle<> handle_;

  bool await_ready() {
    if (!data_.condition) return false;
    bool result = data_.condition();
    if (result) data_.result.result_type = EventType::WAKEUP;
    return result;
  }

  std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) {
    data_.continuation = caller;
    NotifyEmitters(&data_);
    return handle_;
  }

  Result await_resume() { return data_.result; }
};

}  // namespace awaitable
}  // namespace pio

#endif  // PIORUN_COROUTINE_AWAITABLE_UNIVERSAL_H_