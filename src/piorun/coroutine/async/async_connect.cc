#include "coroutine/async/async_connect.h"

#include "coroutine/awaitable/event.h"
#include "coroutine/scheduler.h"
#include "coroutine/task/chainable.h"
#include "socket.h"

namespace pio {

task::Chainable AsyncConnect(SocketView s) {
  int ret = connect(s->fd_, s->addr_.addr(), s->addr_.GetLen());
  if (ret == -1 && errno != EINPROGRESS)
    co_return awaitable::Result{EventType::ERROR, errno,
                                "Failed to connect to server."};
  if (ret == 0) co_return awaitable::Result{EventType::WAKEUP, 0, ""};
  auto epoll = co_await MainScheduler().Event(EventCategory::EPOLL, s->fd_,
                                              s.deadline());
  co_return epoll;
}

}  // namespace pio
