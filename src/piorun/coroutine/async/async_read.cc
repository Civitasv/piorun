#include "coroutine/async/async_read.h"

#include "coroutine/awaitable/event.h"
#include "coroutine/scheduler.h"
#include "coroutine/task/chainable.h"
#include "epoll/epoll_event.h"
#include "socket.h"

namespace pio {

task::Chainable AsyncRead(SocketView s, std::span<std::byte>& data) {
  auto shifting_data = data;
  while (shifting_data.size() > 0) {
    ssize_t cnt = read(s->fd_, shifting_data.data(), shifting_data.size());
    if (cnt == -1 && errno != EAGAIN)
      co_return awaitable::Result{EventType::ERROR, errno,
                                  "Failed to read data from socket."};
    if (cnt > 0) {
      shifting_data = shifting_data.last(shifting_data.size() - cnt);
      continue;
    }
    if (cnt == 0)  // EOF
      break;
    // cnt == -1 && errno == EAGAIN
    if (auto status = co_await MainScheduler().Event(EventCategory::EPOLL,
                                                     s->fd_, s.deadline());
        !status) {
      co_return status;
    }
  }

  data = data.first(data.size() - shifting_data.size());
  co_return awaitable::Result{EventType::WAKEUP, 0, ""};
}

task::Chainable AsyncRead(SocketView s, char* buffer) {
  if (auto status = co_await MainScheduler().Event(EventCategory::EPOLL, s->fd_,
                                                   s.deadline());
      !status) {
    co_return status;
  }
  ssize_t nread;
  char* buf = buffer;
  while (true) {
    nread = read(s->fd_, buf, sizeof(buffer));
    if (nread == 0) {
      break;
    } else if (nread < 0) {
      if (errno == EAGAIN) {
        break;
      } else {
        co_return awaitable::Result{EventType::ERROR, errno,
                                    "Failed to read data from socket."};
      }
    } else {
      buf += nread;
    }
  }
  EpollResetSocket(s->fd_, EPOLLOUT);
  co_return awaitable::Result{EventType::WAKEUP, 0, ""};
}
}  // namespace pio
