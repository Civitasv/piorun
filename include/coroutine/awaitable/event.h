#ifndef PIORUN_COROUTINE_AWAITABLE_DATA_H_
#define PIORUN_COROUTINE_AWAITABLE_DATA_H_

#include <coroutine>
#include <functional>
#include <iosfwd>
#include <ostream>
#include <string>

#include "utils/event_info.h"
#include "utils/time_fwd.h"

namespace pio {
namespace awaitable {
struct Result {
  EventType result_type;
  int err;                  ///< 0 for OK, otherwise errno.
  std::string err_message;  ///< Error message if any.
  constexpr operator bool() const { return result_type == EventType::WAKEUP; }
  friend std::ostream &operator<<(std::ostream &os, const Result &res);
};

struct Event {
  Event(std::function<bool()> condition, TimePoint deadline)
      : continuation{std::noop_coroutine()},  // std::noop_coroutine
                                              // 作为占位符使用
        condition(std::move(condition)),
        event_category(EventCategory::NONE),
        event_id(0),
        deadline(deadline),
        result{} {}

  Event(EventCategory cat, EventID id, TimePoint deadline)
      : continuation{std::noop_coroutine()},
        condition{nullptr},
        event_category(cat),
        event_id(id),
        deadline(deadline),
        result{} {}

  std::coroutine_handle<> continuation;
  std::function<bool()> condition;
  EventCategory event_category;
  EventID event_id;
  TimePoint deadline;
  Result result;
};
}  // namespace awaitable
}  // namespace pio

#endif  // PIORUN_COROUTINE_AWAITABLE_DATA_H_
