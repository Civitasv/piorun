#ifndef PIORUN_CORE_SCHEDULER_H_
#define PIORUN_CORE_SCHEDULER_H_

#include <sys/epoll.h>

#include <stdexcept>

#include "lazy.h"
#include "log.h"
#include "promise.h"

namespace pio {
auto static logger = pio::Logger::Create("piorun_scheduler.log");
class Scheduler {
 private:
  int epoll_fd_;

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

 private:
  // struct LazyHandle {
  //   std::coroutine_handle<> coro_;
  //   int fd_;

  //   LazyHandle(std::coroutine_handle<> coro, int fd) : coro_(coro), fd_(fd)
  //   {}

  //   void Resume() {
  //     if (!coro_.done()) {
  //       coro_.resume();
  //     }
  //   }
  // };

  // template <typename Awaitable>
  // LazyHandle* CreateLazyHandle(Awaitable&& awaitable, int fd) {
  //   auto coro = std::coroutine_handle<>::from_promise(awaitable.promise());
  //   return new LazyHanle(coro, fd);
  // }
};

}  // namespace pio

#endif