#ifndef PIORUN_EPOLL_EPOLL_EVENT_H_
#define PIORUN_EPOLL_EPOLL_EVENT_H_

namespace pio {

void EpollRegisterSocket(int fd);
void EpollDeregisterSocket(int fd);

}  // namespace pio

#endif  // PIORUN_EPOLL_EPOLL_EVENT_H_