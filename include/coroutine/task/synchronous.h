#ifndef PIORUN_COROUTINE_TASK_SYNCHRONOUS_H_
#define PIORUN_COROUTINE_TASK_SYNCHRONOUS_H_

#include <coroutine>
#include <exception>

namespace pio {
namespace task {
struct Synchronous {
  struct promise_type {
    Synchronous get_return_object() { return {}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void unhandled_exception() { std::terminate(); }
    void return_void() {}
  };
};
}  // namespace task
}  // namespace pio

#endif  // PIORUN_COROUTINE_TASK_SYNCHRONOUS_H_
