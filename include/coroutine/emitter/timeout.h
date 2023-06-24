#ifndef PIORUN_COROUTINE_EMITTER_TIMEOUT_H_
#define PIORUN_COROUTINE_EMITTER_TIMEOUT_H_

#include <set>

#include "base.h"
#include "coroutine/awaitable/data.h"

namespace pio {
namespace emitter {

struct Timeout : public Base {
  Timeout() : queue_(CMPDeadline) {}
  awaitable::Data *Emit() override;
  void NotifyArrival(awaitable::Data *data) override;
  void NotifyDeparture(awaitable::Data *data) override;
  bool IsEmpty() override;

  virtual ~Timeout() {}

 private:
  static bool CMPDeadline(awaitable::Data *l, awaitable::Data *r);

  // 一个 data 可能注册了多个 Timeout 事件
  std::multiset<awaitable::Data *, decltype(&CMPDeadline)> queue_;
};

}  // namespace emitter
}  // namespace pio

#endif  // PIORUN_COROUTINE_EMITTER_TIMEOUT_H_