#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>

#include <functional>
#include <iostream>

#include "fiber/fiber.h"
#include "http/httpconn.h"

using namespace pio;
using namespace pio::fiber;

int stop = 0;

static void sighdr(int x) { stop = 1; }

static int SetNonBlock(int iSock) {
  int iFlags;

  iFlags = fcntl(iSock, F_GETFL, 0);
  iFlags |= O_NONBLOCK;
  iFlags |= O_NDELAY;
  int ret = fcntl(iSock, F_SETFL, iFlags);
  return ret;
}

static void SetAddr(const char *pszIP, const unsigned short shPort,
                    sockaddr_in &addr) {
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(shPort);
  int nIP = 0;
  if (!pszIP || '\0' == *pszIP || 0 == strcmp(pszIP, "0") ||
      0 == strcmp(pszIP, "0.0.0.0") || 0 == strcmp(pszIP, "*")) {
    nIP = htonl(INADDR_ANY);
  } else {
    nIP = inet_addr(pszIP);
  }
  addr.sin_addr.s_addr = nIP;
}

static int CreateTcpSocket(const unsigned short shPort, const char *pszIP) {
  int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd >= 0 && shPort != 0) {
    struct sockaddr_in addr;
    SetAddr(pszIP, shPort, addr);
    int ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret != 0) {
      close(fd);
      return -1;
    }
  }
  return fd;
}

void HttpServer() {
  int listenfd = CreateTcpSocket(1234, "127.0.0.1");
  SetNonBlock(listenfd);
  listen(listenfd, 1024);

  assert(listenfd >= 0);

  while (!stop) {
    sockaddr_in addr;
    socklen_t len = sizeof(addr);

    int clifd = accept(listenfd, (sockaddr *)&addr, &len);

    if (clifd < 0) {
      // 因为listenfd是非阻塞的，如果accept失败，使用poll监视listenfd是否有新连接可accept.
      // 这里不能使用epoll，使用libco后用户不能随意使用epoll.
      struct pollfd pf = {0};
      pf.fd = listenfd;
      pf.events = (POLLIN | POLLERR | POLLHUP);
      poll(&pf, 1, 1000);
      continue;
    } else {
      char buf[32];
      memset(buf, 0, 32);
      inet_ntop(AF_INET, &addr.sin_addr, buf, 32);
    }

    go[clifd, addr] {
      HttpConn conn = HttpConn();
      conn.init(clifd, addr);
      // timeval timeout;
      // timeout.tv_sec = 60;
      // timeout.tv_usec = 0;
      // setsockopt(clifd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
      do {
        do {
          int ret = -1;
          int readErrno = 0;
          ret = conn.read(&readErrno);
          // printf("ret=%d\n", ret);
          if (ret <= 0 && errno != EAGAIN) {
            conn.Close();
            return;
          }
        } while (!conn.process());
        do {
          int ret = -1;
          int writeErrno = 0;
          ret = conn.write(&writeErrno);
          if (ret <= 0 && writeErrno != EAGAIN) {
            conn.Close();
            return;
          }
        } while (conn.ToWriteBytes() != 0);

      } while (conn.IsKeepAlive());

      conn.Close();
    };
  }

  close(listenfd);
}

#include <iostream>
int main(int argc, const char *agrv[]) {
  signal(SIGQUIT, sighdr);
  auto srcDir_ = getcwd(nullptr, 256);
  strncat(srcDir_, "/resources/", 16);
  HttpConn::userCount = 0;
  HttpConn::srcDir = srcDir_;
  go HttpServer;

  return 0;
}

// #include <arpa/inet.h>
// #include <assert.h>
// #include <fcntl.h>
// #include <signal.h>
// #include <string.h>
// #include <sys/poll.h>
// #include <sys/socket.h>

// #include <functional>
// #include <iostream>

// #include "fiber/fiber.h"
// #include "http/http_conn.h"

// using namespace pio;
// using namespace pio::fiber;

// int stop = 0;

// static void sighdr(int x) { stop = 1; }

// static int SetNonBlock(int iSock) {
//   int iFlags;

//   iFlags = fcntl(iSock, F_GETFL, 0);
//   iFlags |= O_NONBLOCK;
//   iFlags |= O_NDELAY;
//   int ret = fcntl(iSock, F_SETFL, iFlags);
//   return ret;
// }

// static void SetAddr(const char *pszIP, const unsigned short shPort,
//                     sockaddr_in &addr) {
//   bzero(&addr, sizeof(addr));
//   addr.sin_family = AF_INET;
//   addr.sin_port = htons(shPort);
//   int nIP = 0;
//   if (!pszIP || '\0' == *pszIP || 0 == strcmp(pszIP, "0") ||
//       0 == strcmp(pszIP, "0.0.0.0") || 0 == strcmp(pszIP, "*")) {
//     nIP = htonl(INADDR_ANY);
//   } else {
//     nIP = inet_addr(pszIP);
//   }
//   addr.sin_addr.s_addr = nIP;
// }

// static int CreateTcpSocket(const unsigned short shPort, const char *pszIP) {
//   int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//   if (fd >= 0 && shPort != 0) {
//     struct sockaddr_in addr;
//     SetAddr(pszIP, shPort, addr);
//     int ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
//     if (ret != 0) {
//       close(fd);
//       return -1;
//     }
//   }
//   return fd;
// }

// void HttpServer() {
//   int listenfd = CreateTcpSocket(1234, "127.0.0.1");
//   SetNonBlock(listenfd);
//   listen(listenfd, 1024);

//   assert(listenfd >= 0);

//   while (!stop) {
//     sockaddr_in addr;
//     socklen_t len = sizeof(addr);

//     int clifd = accept(listenfd, (sockaddr *)&addr, &len);

//     if (clifd < 0) {
//       //
//       因为listenfd是非阻塞的，如果accept失败，使用poll监视listenfd是否有新连接可accept.
//       // 这里不能使用epoll，使用libco后用户不能随意使用epoll.
//       struct pollfd pf = {0};
//       pf.fd = listenfd;
//       pf.events = (POLLIN | POLLERR | POLLHUP);
//       poll(&pf, 1, 1000);
//       continue;
//     } else {
//       char buf[32];
//       memset(buf, 0, 32);
//       inet_ntop(AF_INET, &addr.sin_addr, buf, 32);
//       // printf("%s\n", buf);
//     }

//     go[clifd] {
//       // printf("new session at: [%d|%p]\n", this_fiber::get_thread_id(),
//       //        this_fiber::co_self());
//       HttpConn conn = HttpConn();
//       while (true) {
//         HttpConn().init();
//         int n = read(clifd, conn.read_buf_, sizeof(conn.read_buf_));
//         if (n <= 0) {
//           close(clifd);
//           break;
//         }
//         conn.Process();
//         size_t nleft = strlen(conn.write_buf_);
//         char *bufp = conn.write_buf_;
//         ssize_t nwritten;

//         while (nleft > 0) {
//           if ((nwritten = write(clifd, conn.write_buf_, nleft)) <= 0) {
//             if (errno == EINTR ||
//                 errno == EAGAIN) /* Interrupted by sig handler return */
//               nwritten = 0;      /* and call write() again */
//             else
//               return; /* errno set by write() */
//           }
//           nleft -= nwritten;
//           bufp += nwritten;
//         }
//       }
//     };
//   }

//   close(listenfd);
// }

// int main(int argc, const char *agrv[]) {
//   signal(SIGQUIT, sighdr);

//   go HttpServer;

//   return 0;
// }