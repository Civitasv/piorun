#ifndef PIORUN_COROUTINE_SCHEDULER_H_
#define PIORUN_COROUTINE_SCHEDULER_H_

#include <sys/epoll.h>

#include <coroutine>
#include <deque>
#include <list>
#include <stdexcept>

#include "core/log.h"
#include "core/smartptr.h"
#include "coroutine/awaitable/event.h"
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
struct Scheduler {
  static constexpr struct GetPromiseTag {
  } GetPromise;

  struct promise_type {
    Scheduler get_return_object() { return Scheduler{this}; }
    // 这里堵塞，直到我们手动调用 `SchedulerTask.Run`
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void unhandled_exception() { std::terminate(); }

    awaitable::Handoff yield_value(awaitable::Event *event) {
      if (event == nullptr) return awaitable::Handoff(std::noop_coroutine());

      for (auto &em : emitters_) {
        // 取消对 事件 的监听
        em->NotifyDeparture(event);
      }
      // 从 continuation 点继续执行
      return awaitable::Handoff(event->continuation);
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

    // 要调度的任务
    std::deque<std::coroutine_handle<>> scheduled_;
    // 每个任务会生产多个事件，这些事件会加入到 emitter 的事件监听队列
    // 每一次，emitter 会调用 Emit，处理队列中的一个事件，并返回事件处理结果
    std::vector<Scope<emitter::Base>> emitters_;
  };

  using Handle = std::coroutine_handle<promise_type>;

  explicit Scheduler(promise_type *p)
      : coro_handle_(Handle::from_promise(*p)) {}

  Scheduler(const Scheduler &) = delete;
  Scheduler(Scheduler &&) = delete;
  Scheduler &operator=(const Scheduler &) = delete;
  Scheduler &operator=(Scheduler &&) = delete;

  void Schedule(CoroutineWithContinuation auto &coro) {
    // 将该 coroutine 添加到 Schedule 中
    coro_handle_.promise().scheduled_.push_back(coro.coro_handle());
    // 设置该 coroutine 的继续点为当前 handler
    coro.coro_handle().promise().set_continuation(coro_handle_);
  }

  void RegisterEmitter(Scope<emitter::Base> emitter);
  void Run() { coro_handle_.resume(); }

  // 这里均返回 awaitable::Universal
  // 而在其中的 await_suspend 方法中，会调用 NotifyEmitters
  // 从而将事件添加到 emitters 事件队列之中
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
  void NotifyEmitters(awaitable::Event *);
  friend void awaitable::NotifyEmitters(awaitable::Event *);
};

Scheduler &MainScheduler();

}  // namespace pio

#endif  // PIORUN_COROUTINE_SCHEDULER_H_
