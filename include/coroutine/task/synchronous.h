#ifndef PIORUN_COROUTINE_TASK_SYNCHRONOUS_H_
#define PIORUN_COROUTINE_TASK_SYNCHRONOUS_H_

#include <coroutine>
#include <exception>

namespace pio {
namespace task {
/**
 * @brief 由于 initial_suspend 和 final_suspend 都返回 suspend_never
 *        所以，该协程不会挂起，相当于普通的函数
 */
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
