#include "coroutine/emitter/epoll.h"

#include <sys/epoll.h>
#include <sys/socket.h>

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <system_error>

namespace pio {
namespace emitter {

namespace {

int GetErrno(int fd) {
  int error = 0;
  socklen_t errlen = sizeof(error);
  if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen) == 0) {
    return error;
  } else {
    return errno;
  }
}

}  // namespace

int Epoll::epoll_fd = -1;

// When calling epoll_create, the paramter is ignored, but has to be >0
constexpr int IGNORED_EPOLL_PARAM = 1;

Epoll::Epoll() {
  if (epoll_fd != -1) return;
  epoll_fd = epoll_create(IGNORED_EPOLL_PARAM);
  if (epoll_fd == -1)
    throw std::system_error(errno, std::system_category(),
                            "Failed to create epoll socket.");
}

awaitable::Event *Epoll::Emit() {
  epoll_event ev;
  while (true) {
    int ret = epoll_wait(epoll_fd, &ev, 1, 0);
    if (ret == -1)
      throw std::system_error(errno, std::system_category(),
                              "Failed to fetch epoll event.");

    if (ret == 0) return nullptr; // timeout

    auto it = awaiting_.find(ev.data.fd);
    if (it == awaiting_.end()) continue;

    // epoll_event_dump(ev);
    if (ev.events & EPOLLERR) {  // 错误
      it->second->result.err = GetErrno(ev.data.fd);
      it->second->result.result_type = EventType::ERROR;
      it->second->result.err_message = "Asynchronous socket error.";
    } else if (ev.events & (EPOLLHUP | EPOLLRDHUP)) {  // 读或写关闭
      it->second->result.err = 0;
      it->second->result.err_message = "";
      it->second->result.result_type = EventType::HANGUP;
    } else if (ev.events & (EPOLLIN | EPOLLOUT)) {  // 读或写事件
      it->second->result.err = 0;
      it->second->result.err_message = "";
      it->second->result.result_type = EventType::WAKEUP;
    }
    return it->second;
  }
}

void Epoll::NotifyArrival(awaitable::Event *data) {
  if (data->event_category != EventCategory::EPOLL) return;
  awaiting_.emplace(data->event_id, data);
}

void Epoll::NotifyDeparture(awaitable::Event *data) {
  if (data->event_category != EventCategory::EPOLL) return;
  awaiting_.erase(data->event_id);
}

bool Epoll::IsEmpty() { return awaiting_.empty(); }

}  // namespace emitter
}  // namespace pio