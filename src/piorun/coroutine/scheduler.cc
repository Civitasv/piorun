#include "coroutine/scheduler.h"

#include "coroutine/awaitable/data.h"
#include "utils/time_fwd.h"

namespace pio {

void SchedulerTask::RegisterEmitter(Scope<emitter::Base> emitter) {
  coro_handle_.promise().emitters_.push_back(std::move(emitter));
}

namespace {
SchedulerTask MainScheduleIml() {
  auto &promise = co_await SchedulerTask::GetPromise;
  while (true) {
    while (promise.scheduled_.size() > 0) {
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
        co_yield ev;
      }
    }
  }
}
}  // namespace

SchedulerTask &MainScheduler() {
  static SchedulerTask scheduler = MainScheduleIml();
  return scheduler;
}

void awaitable::NotifyEmitters(awaitable::Data *data) {
  MainScheduler().NotifyEmitters(data);
}

void SchedulerTask::NotifyEmitters(awaitable::Data *data) {
  for (auto &em : coro_handle_.promise().emitters_) {
    em->NotifyArrival(data);
  }
}

awaitable::Universal SchedulerTask::Event(EventCategory category, EventID id,
                                          Duration timeout) {
  return Event(category, id, Clock::now() + timeout);
}

awaitable::Universal SchedulerTask::Event(EventCategory category, EventID id,
                                          TimePoint deadline) {
  return awaitable::Universal{awaitable::Data(category, id, deadline),
                              coro_handle_};
}

awaitable::Universal SchedulerTask::Condition(std::function<bool()> cond,
                                              Duration timeout) {
  return Condition(std::move(cond), Clock::now() + timeout);
}

awaitable::Universal SchedulerTask::Condition(std::function<bool()> cond,
                                              TimePoint deadline) {
  return awaitable::Universal{awaitable::Data(std::move(cond), deadline),
                              coro_handle_};
}

}  // namespace pio