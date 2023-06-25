#include "coroutine/scheduler.h"

#include "coroutine/awaitable/event.h"
#include "utils/time_fwd.h"

namespace pio {

void Scheduler::RegisterEmitter(Scope<emitter::Base> emitter) {
  coro_handle_.promise().emitters_.push_back(std::move(emitter));
}

namespace {
Scheduler MainScheduleIml() {
  auto &promise = co_await Scheduler::GetPromise;
  while (true) {
    while (promise.scheduled_.size() > 0) {
      // 控制权交给 first task of scheduled tasks
      co_await awaitable::Handoff(promise.scheduled_.front());
      if (promise.scheduled_.front().done()) {
        promise.scheduled_.front().destroy();
      }
      promise.scheduled_.pop_front();
    }

    bool all_done = true;
    for (auto &em : promise.emitters_) {
      if (!em->IsEmpty()) {
        all_done = false;
      }
    }
    if (all_done) {
      co_yield nullptr;
    }

    for (auto &em : promise.emitters_) {
      for (auto *ev = em->Emit(); ev != nullptr; ev = em->Emit()) {
        co_yield ev;  // 得到该事件的处理结果，并通过 co_yield，取消 emitter
                      // 对该事件的监听
      }
    }
  }
}
}  // namespace

Scheduler &MainScheduler() {
  static Scheduler scheduler = MainScheduleIml();
  return scheduler;
}

void awaitable::NotifyEmitters(awaitable::Event *event) {
  MainScheduler().NotifyEmitters(event);
}

void Scheduler::NotifyEmitters(awaitable::Event *event) {
  for (auto &em : coro_handle_.promise().emitters_) {
    em->NotifyArrival(event);
  }
}

awaitable::Universal Scheduler::Event(EventCategory category, EventID id,
                                      Duration timeout) {
  return Event(category, id, Clock::now() + timeout);
}

awaitable::Universal Scheduler::Event(EventCategory category, EventID id,
                                      TimePoint deadline) {
  return awaitable::Universal{awaitable::Event(category, id, deadline),
                              coro_handle_};
}

awaitable::Universal Scheduler::Condition(std::function<bool()> cond,
                                          Duration timeout) {
  return Condition(std::move(cond), Clock::now() + timeout);
}

awaitable::Universal Scheduler::Condition(std::function<bool()> cond,
                                          TimePoint deadline) {
  return awaitable::Universal{awaitable::Event(std::move(cond), deadline),
                              coro_handle_};
}

}  // namespace pio