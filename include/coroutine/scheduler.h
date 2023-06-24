#ifndef PIORUN_COROUTINE_SCHEDULER_H_
#define PIORUN_COROUTINE_SCHEDULER_H_

#include <sys/epoll.h>

#include <coroutine>
#include <deque>
#include <list>
#include <stdexcept>

#include "core/log.h"
#include "core/smartptr.h"
#include "coroutine/awaitable/data.h"
#include "coroutine/awaitable/handoff.h"
#include "coroutine/awaitable/universal.h"
#include "coroutine/emitter/base.h"
#include "utils/concepts.h"
#include "utils/event_info.h"
#include "utils/time_fwd.h"

namespace pio {
auto static logger = pio::Logger::Create("piorun_scheduler.log");

/**
 * @brief The scheduler is a generator style coroutine
 *        that keeps looping and yielding control to
 *        coroutines that can run.
 */
struct SchedulerTask {
  static constexpr struct GetPromiseTag {
  } GetPromise;

  struct promise_type {
    SchedulerTask get_return_object() { return SchedulerTask{this}; }
    // 这里堵塞，直到我们手动调用 `SchedulerTask.Run`
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void unhandled_exception() { std::terminate(); }

    awaitable::Handoff yield_value(awaitable::Data *data) {
      if (data == nullptr) return awaitable::Handoff(std::noop_coroutine());

      for (auto &em : emitters_) {
        // 对 data 取消该事件
        em->NotifyDeparture(data);
      }
      // 从 continuation 点继续执行
      return awaitable::Handoff(data->continuation);
    }

    auto await_transform(GetPromiseTag) {
      struct Awaiter : std::suspend_never {
        promise_type &promise;

        promise_type &await_resume() { return promise; }
      };
      return Awaiter{{}, *this};
    }

    template <typename U>
    U &&await_transform(U &&awaitable) noexcept {
      return std::forward<U &&>(awaitable);
    }

    std::deque<std::coroutine_handle<>> scheduled_;
    std::vector<Scope<emitter::Base>> emitters_;
  };

  using Handle = std::coroutine_handle<promise_type>;

  explicit SchedulerTask(promise_type *p)
      : coro_handle_(Handle::from_promise(*p)) {}

  SchedulerTask(const SchedulerTask &) = delete;
  SchedulerTask(SchedulerTask &&) = delete;
  SchedulerTask &operator=(const SchedulerTask &) = delete;
  SchedulerTask &operator=(SchedulerTask &&) = delete;

  void Schedule(CoroutineWithContinuation auto &coro) {
    // 将该 coroutine 添加到 Schedule 中
    coro_handle_.promise().scheduled_.push_back(coro.coro_handle());
    // 设置该 coroutine 的继续点为当前 handler
    coro.coro_handle().promise().set_continuation(coro_handle_);
  }

  void RegisterEmitter(Scope<emitter::Base> emitter);
  void Run() { coro_handle_.resume(); }

  awaitable::Universal Event(EventCategory category, EventID id,
                             Duration timeout);
  awaitable::Universal Event(EventCategory category, EventID id,
                             TimePoint deadline = NO_DEADLINE);
  awaitable::Universal Condition(std::function<bool()> condition,
                                 Duration timeout);
  awaitable::Universal Condition(std::function<bool()> condition,
                                 TimePoint deadline = NO_DEADLINE);

 private:
  Handle coro_handle_;
  void NotifyEmitters(awaitable::Data *);
  friend void awaitable::NotifyEmitters(awaitable::Data *);
};

SchedulerTask &MainScheduler();

}  // namespace pio

#endif  // PIORUN_COROUTINE_SCHEDULER_H_
