#include "coroutine/emitter/timeout.h"

namespace pio {
namespace emitter {

awaitable::Data *Timeout::Emit() {
  if (queue_.size() != 0) {
    auto v = *queue_.begin();
    if (v->deadline < Clock::now()) {
      v->result.result_type = EventType::TIMEOUT;
      v->result.err = ETIMEDOUT;
      v->result.err_message = "Timed out by TimeoutEmitter.";
      return v;
    }
  }
  return nullptr;
}

void Timeout::NotifyArrival(awaitable::Data *data) {
  if (data->deadline != NO_DEADLINE) queue_.insert(data);
}

void Timeout::NotifyDeparture(awaitable::Data *data) {
  auto [b, e] = queue_.equal_range(data);
  for (auto i = b; i != e; i++) {
    if (*i == data) {
      queue_.erase(i);
      return;
    }
  }
}

bool Timeout::CMPDeadline(awaitable::Data *l, awaitable::Data *r) {
  return l->deadline < r->deadline;
}

bool Timeout::IsEmpty() { return queue_.empty(); }

}  // namespace emitter
}  // namespace pio