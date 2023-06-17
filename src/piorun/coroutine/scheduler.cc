#include "coroutine/scheduler.h"

namespace pio {
void Scheduler::AddLazy(std::coroutine_handle<> coro_handle, int fd) {
  epoll_event event{};
  event.events = EPOLLIN | EPOLLET;
  event.data.ptr = coro_handle.address();
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) == -1) {
    throw std::runtime_error("Failed to add lazy to epoll");
    logger->Error("Failed to add lazy to epoll");
  }
}

void Scheduler::Run() {
  epoll_event events[1024];
  while (true) {
    int events_num = epoll_wait(epoll_fd_, events, 1024, -1);
    for (int i = 0; i < events_num; ++i) {
      auto handle = std::coroutine_handle<>::from_address(
          events[i].data.ptr);  // 获取协程句柄
      if (handle) {
        handle.resume();
      }
    }
  }
}
}  // namespace pio