#ifndef PIORUN_COROUTINE_EMITTER_TIMEOUT_H_
#define PIORUN_COROUTINE_EMITTER_TIMEOUT_H_

#include <set>

#include "base.h"
#include "coroutine/awaitable/event.h"

namespace pio {
namespace emitter {

/**
 * @brief 超时事件监听
 */
struct Timeout : public Base {
  Timeout() : queue_(CMPDeadline) {}
  awaitable::Event *Emit() override;
  void NotifyArrival(awaitable::Event *event) override;
  void NotifyDeparture(awaitable::Event *event) override;
  bool IsEmpty() override;

  virtual ~Timeout() {}

 private:
  static bool CMPDeadline(awaitable::Event *l, awaitable::Event *r);

  // 相同的 event 可重复添加
  std::multiset<awaitable::Event *, decltype(&CMPDeadline)> queue_;
};

}  // namespace emitter
}  // namespace pio

#endif  // PIORUN_COROUTINE_EMITTER_TIMEOUT_H_