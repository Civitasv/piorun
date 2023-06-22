#ifndef PIORUN_COROUTINE_TASK_H_
#define PIORUN_COROUTINE_TASK_H_

#include <coroutine>
#include <utility>

namespace pio {

class Task {
 public:
  struct promise_type {
    Task get_return_object() { return {}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
  };
};
}  // namespace pio

#endif  // PIORUN_COROUTINE_TASK_H_
