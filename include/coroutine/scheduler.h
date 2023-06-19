#ifndef PIORUN_COROUTINE_SCHEDULER_H_
#define PIORUN_COROUTINE_SCHEDULER_H_

#include <sys/epoll.h>

#include <stdexcept>

#include "core/log.h"
#include "lazy.h"
#include "promise.h"

namespace pio {
auto static logger = pio::Logger::Create("piorun_scheduler.log");

class Scheduler {
 public:
  Scheduler() : epoll_fd_(-1) {
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ == -1) {
      throw std::runtime_error("Failed to create epoll");
      logger->Error("Failed to create epoll");
    }
  }

  ~Scheduler() {
    if (epoll_fd_ != -1) {
      close(epoll_fd_);
    }
  }

  void AddLazy(std::coroutine_handle<> coro_handle, int fd);

  void Run();

 public:
  int listen_fd_;
  int epoll_fd_;
};

}  // namespace pio

#endif  // PIORUN_COROUTINE_SCHEDULER_H_
