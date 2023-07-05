#include "epoll/epoll_event.h"

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <iostream>
#include <stdexcept>
#include <system_error>

#include "coroutine/emitter/epoll.h"

namespace pio {

void EpollRegisterSocket(int fd) {
  int flag = fcntl(fd, F_GETFL);
  flag |= O_NONBLOCK;
  fcntl(fd, F_SETFL, flag);
  epoll_event ev = {};
  ev.events = EPOLLIN | EPOLLOUT | EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLET;
  ev.data.fd = fd;
  int ret = epoll_ctl(emitter::Epoll::epoll_fd, EPOLL_CTL_ADD, fd, &ev);
  if (ret == -1)
    throw std::system_error(errno, std::system_category(),
                            "Failed to register socket for epoll.");
}

void EpollDeregisterSocket(int fd) {
  int ret = epoll_ctl(emitter::Epoll::epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
  if (ret == -1)
    throw std::system_error(errno, std::system_category(),
                            "Failed to de-register socket from epoll.");
}

void EpollResetSocket(int fd, int ev) {
  epoll_event event;
  event.data.fd = fd;
  event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
  epoll_ctl(emitter::Epoll::epoll_fd, EPOLL_CTL_MOD, fd, &event);
}

}  // namespace pio