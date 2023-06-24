#include "coroutine/async/async_accept.h"

#include "coroutine/scheduler.h"

namespace pio {

task::Chainable AsyncAccept(
    SocketView s, std::function<task::Terminating(Socket)> connection_handler) {
  int ret;
  while ((ret = accept4(s->fd_, nullptr, nullptr, SOCK_NONBLOCK)) == -1) {
    if (errno != EAGAIN && errno != EWOULDBLOCK)
      co_return awaitable::Result{EventType::ERROR, errno,
                                  "Failed to accept connection to server."};

    auto status = co_await MainScheduler().Event(EventCategory::EPOLL, s->fd_,
                                                 s.deadline());
    if (status.result_type != EventType::WAKEUP) co_return status;
  }
  auto handle = connection_handler(Socket::AcceptedSocket(ret));
  MainScheduler().Schedule(handle);
  co_return awaitable::Result{};
}

}  // namespace pio
