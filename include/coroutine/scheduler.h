#ifndef PIORUN_COROUTINE_SCHEDULER_H_
#define PIORUN_COROUTINE_SCHEDULER_H_

#include <sys/epoll.h>

#include <coroutine>
#include <list>
#include <stdexcept>

#include "core/log.h"

namespace pio {
auto static logger = pio::Logger::Create("piorun_scheduler.log");

class Scheduler {
 private:
  std::list<std::coroutine_handle<>> tasks_{};

 public:
  bool Schedule() {
    auto task = tasks_.front();
    tasks_.pop_front();

    if (not task.done()) {
      // FIXME. 这里的唤醒策略需要修改为当 task 完成时，对 task 进行 resume
      task.resume();
    }

    return not tasks_.empty();
  }

  auto Suspend() {
    struct Awaiter : std::suspend_always {
      Scheduler& sched_;

      explicit Awaiter(Scheduler& sched) : sched_(sched) {}

      void await_suspend(std::coroutine_handle<> coro) const noexcept {
        sched_.tasks_.push_back(coro);
      }
    };

    return Awaiter(*this);
  }
};

}  // namespace pio

#endif  // PIORUN_COROUTINE_SCHEDULER_H_
