#ifndef PIORUN_SOCKET_H_
#define PIORUN_SOCKET_H_

#include <arpa/inet.h>

#include <variant>

#include "utils/observer.h"

namespace pio {

struct IPv4 {};
struct IPv6 {};

struct SockAddr {
  SockAddr(IPv4, const char *addr, int port);
  SockAddr(IPv6, const char *addr, int port);
  SockAddr() {}

  const struct sockaddr *addr() const;
  socklen_t GetLen() const;

  std::variant<sockaddr_in, sockaddr_in6> addr_;
};

struct Socket;

using SocketView = ObserverWithDeadline<Socket>;

struct Socket {
  static Socket ServerSocket(SockAddr addr);
  static Socket ClientSocket(SockAddr addr);
  static Socket AcceptedSocket(int fd);

  SocketView WithoutTimeout() { return SocketView(*this); }
  SocketView WithTimeout(Duration timeout) {
    return SocketView(*this, Clock::now() + timeout);
  }

  Socket(SockAddr addr);
  Socket(int fd);
  ~Socket();
  Socket(const Socket &) = delete;
  Socket(Socket &&s)
      : addr_(std::exchange(s.addr_, SockAddr{})),
        fd_(std::exchange(s.fd_, -1)) {}

  Socket &operator=(const Socket &s) = delete;
  Socket &operator=(Socket &&s) {
    addr_ = std::exchange(s.addr_, SockAddr{});
    fd_ = std::exchange(s.fd_, -1);
    return *this;
  }

  Socket MakeClient();

  SockAddr addr_;
  int fd_;
  bool operational_ = false;
};

}  // namespace pio

#endif  // PIORUN_SOCKET_H_