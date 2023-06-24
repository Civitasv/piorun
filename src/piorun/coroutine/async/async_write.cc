#include "coroutine/async/async_write.h"

#include "coroutine/awaitable/data.h"
#include "coroutine/scheduler.h"
#include "coroutine/task/chainable.h"
#include "socket.h"

namespace pio {

task::Chainable AsyncWrite(SocketView s, std::span<const std::byte> &data) {
  while (data.size() > 0) {
    ssize_t cnt = write(s->fd_, data.data(), data.size());
    if (cnt == -1 && errno != EAGAIN)
      co_return awaitable::Result{EventType::ERROR, errno,
                                  "Failed to write data to socket."};
    if (cnt >= 0) {
      data = data.last(data.size() - cnt);
      continue;
    }
    // cnt == -1 && errno == EAGAIN
    if (auto status = co_await MainScheduler().Event(EventCategory::EPOLL,
                                                     s->fd_, s.deadline());
        !status) {
      co_return status;
    }
  }
  co_return awaitable::Result{EventType::WAKEUP, 0, ""};
}

}  // namespace pio
