#include "coroutine/async/async_write.h"

#include "coroutine/awaitable/event.h"
#include "coroutine/scheduler.h"
#include "coroutine/task/chainable.h"
#include "socket.h"

namespace pio {

task::Chainable AsyncWrite(SocketView s, std::span<const std::byte>& data) {
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

// write其实不需要等待，因为写缓存区基本都是可写的
// 并且如果将写操作也注册为事件，会出现bug
// 主要就在于epoll_wait可能感知不到socket可写，所以并不会执行真正的写操作
// 因此client就会一直被搁置
task::Chainable AsyncWrite(SocketView s, char* buffer, size_t n) {
  // if (auto status = co_await MainScheduler().Event(EventCategory::EPOLL,
  // s->fd_,
  //                                                  s.deadline());
  //     !status) {
  //   co_return status;
  // }
  size_t nleft = n;
  ssize_t nwritten;
  char* buf = buffer;
  while (nleft > 0) {
    nwritten = write(s->fd_, buf, nleft);
    if (nwritten < 0) {
      if (errno == EAGAIN) {
        break;
      } else {
        co_return awaitable::Result{EventType::ERROR, errno,
                                    "Failed to write data to socket."};
      }
    } else {
      nleft -= nwritten;
      buf += nwritten;
    }
  }
  co_return awaitable::Result{EventType::WAKEUP, 0, ""};
}

}  // namespace pio
