#ifndef PIORUN_COROUTINE_EMITTER_EPOLL_H_
#define PIORUN_COROUTINE_EMITTER_EPOLL_H_

#include <set>
#include <unordered_map>

#include "base.h"
#include "coroutine/awaitable/event.h"

namespace pio {
namespace emitter {

struct Epoll : public Base {
  static int epoll_fd;  ///< epoll file descriptor.

  Epoll();

  /**
   * @brief 使用 epoll 机制监听 awaiting_ 中的 fd
   *        若有事件就绪，则返回该事件，否则返回 nullptr
   * @return awaitable::Data*
   */
  awaitable::Event *Emit() override;
  void NotifyArrival(awaitable::Event *event) override;
  void NotifyDeparture(awaitable::Event *event) override;
  bool IsEmpty() override;

  virtual ~Epoll() {}

 private:
  std::unordered_map<int, awaitable::Event *> awaiting_;  ///< fd->data
};

}  // namespace emitter
}  // namespace pio

#endif  // PIORUN_COROUTINE_EMITTER_EPOLL_H_