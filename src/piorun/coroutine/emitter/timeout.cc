#include "coroutine/emitter/timeout.h"

namespace pio {
namespace emitter {

awaitable::Event *Timeout::Emit() {
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

void Timeout::NotifyArrival(awaitable::Event *data) {
  if (data->deadline != NO_DEADLINE) queue_.insert(data);
}

void Timeout::NotifyDeparture(awaitable::Event *data) {
  auto [b, e] = queue_.equal_range(data);
  for (auto i = b; i != e; i++) {
    if (*i == data) {
      queue_.erase(i);
      return;
    }
  }
}

bool Timeout::CMPDeadline(awaitable::Event *l, awaitable::Event *r) {
  return l->deadline < r->deadline;
}

bool Timeout::IsEmpty() { return queue_.empty(); }

}  // namespace emitter
}  // namespace pio