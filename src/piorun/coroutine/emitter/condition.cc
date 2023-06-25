#include "coroutine/emitter/condition.h"

#include <algorithm>

namespace pio {
namespace emitter {

awaitable::Event *Condition::Emit() {
  auto it =
      std::ranges::find_if(awaiting_, [](auto *d) { return d->condition(); });
  if (it == awaiting_.end()) return nullptr;
  (*it)->result.err = 0;
  (*it)->result.err_message = "";
  (*it)->result.result_type = EventType::WAKEUP;
  return *it;
}

void Condition::NotifyArrival(awaitable::Event *data) {
  if (data->condition) {
    awaiting_.insert(data);
  }
}

void Condition::NotifyDeparture(awaitable::Event *data) {
  if (data->condition) {
    awaiting_.erase(data);
  }
}

bool Condition::IsEmpty() { return awaiting_.empty(); }

}  // namespace emitter
}  // namespace pio