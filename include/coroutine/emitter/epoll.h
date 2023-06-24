#ifndef PIORUN_COROUTINE_EMITTER_EPOLL_H_
#define PIORUN_COROUTINE_EMITTER_EPOLL_H_

#include <set>
#include <unordered_map>

#include "base.h"
#include "coroutine/awaitable/data.h"

namespace pio {
namespace emitter {

struct Epoll : public Base {
  static int epoll_fd;

  Epoll();
  awaitable::Data *Emit() override;
  void NotifyArrival(awaitable::Data *data) override;
  void NotifyDeparture(awaitable::Data *data) override;
  bool IsEmpty() override;

  virtual ~Epoll() {}


 private:
  std::unordered_map<int, awaitable::Data *> awaiting_;
};

}  // namespace emitter
}  // namespace pio

#endif  // PIORUN_COROUTINE_EMITTER_EPOLL_H_