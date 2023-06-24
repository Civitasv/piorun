#ifndef PIORUN_COROUTINE_EMITTER_CONDITION_H_
#define PIORUN_COROUTINE_EMITTER_CONDITION_H_

#include <unordered_set>

#include "coroutine/awaitable/data.h"
#include "coroutine/emitter/base.h"

namespace pio {
namespace emitter {

struct Condition : public Base {
  awaitable::Data *Emit() override;

  void NotifyArrival(awaitable::Data *data) override;
  void NotifyDeparture(awaitable::Data *data) override;
  bool IsEmpty() override;

  virtual ~Condition() {}

 private:
  std::unordered_set<awaitable::Data *> awaiting_;
};

}  // namespace emitter
}  // namespace pio

#endif  // PIORUN_COROUTINE_EMITTER_CONDITION_H_