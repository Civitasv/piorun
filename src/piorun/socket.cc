#include "socket.h"

#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <system_error>

#include "epoll/epoll_event.h"

namespace pio {

SockAddr::SockAddr(IPv4, const char *addr, int port) : addr_{sockaddr_in{}} {
  int ret = inet_pton(AF_INET, addr, &(get<sockaddr_in>(addr_).sin_addr));
  if (ret == -1)
    throw std::system_error(errno, std::system_category(),
                            "Failed to convert IPv4 address to binary form.");

  get<sockaddr_in>(addr_).sin_family = AF_INET;
  get<sockaddr_in>(addr_).sin_port = htons(port);
}

SockAddr::SockAddr(IPv6, const char *addr, int port) : addr_{sockaddr_in6{}} {
  int ret = inet_pton(AF_INET6, addr, &(get<sockaddr_in6>(addr_).sin6_addr));
  if (ret == -1)
    throw std::system_error(errno, std::system_category(),
                            "Failed to convert IPv6 address to binary form.");

  get<sockaddr_in6>(addr_).sin6_family = AF_INET6;
  get<sockaddr_in6>(addr_).sin6_port = htons(port);
}

const struct sockaddr *SockAddr::addr() const {
  return std::visit(
      [](auto &addr) { return reinterpret_cast<const sockaddr *>(&addr); },
      addr_);
}

socklen_t SockAddr::GetLen() const {
  return std::visit([](auto &addr) { return sizeof(addr); }, addr_);
}

Socket Socket::ServerSocket(SockAddr addr) {
  Socket s(addr);
  int ret = bind(s.fd_, s.addr_.addr(), s.addr_.GetLen());
  if (ret == -1)
    throw std::system_error(errno, std::system_category(),
                            "Failed to bind socket to address.");
  ret = listen(s.fd_, 4096);
  if (ret == -1)
    throw std::system_error(errno, std::system_category(),
                            "Failed to listen on socket.");
  s.operational_ = true;
  return s;
}

Socket Socket::ClientSocket(SockAddr addr) { return Socket(addr); }
Socket Socket::AcceptedSocket(int fd) { return Socket(fd); }

Socket Socket::MakeClient() { return Socket(addr_); }

Socket::Socket(SockAddr addr) : addr_(addr) {
  int family = 0;
  if (std::holds_alternative<sockaddr_in>(addr.addr_)) {
    family = AF_INET;
  } else if (std::holds_alternative<sockaddr_in6>(addr.addr_)) {
    family = AF_INET6;
  } else {
    throw std::runtime_error("Unknown protocol family.");
  }
  fd_ = socket(family, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (fd_ == -1)
    throw std::system_error(errno, std::system_category(),
                            "Failed to create socket.");
  EpollRegisterSocket(fd_);
}

Socket::Socket(int fd) : fd_(fd) { EpollRegisterSocket(fd_); }

Socket::~Socket() {
  if (fd_ != -1) {
    EpollDeregisterSocket(fd_);
    close(fd_);
  }
}

}  // namespace pio