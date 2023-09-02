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
  int listenfd = CreateTcpSocket(1235, "127.0.0.1");
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
          if (ret <= 0 || errno != EAGAIN) {
            if (ret < 0) {
              printf("error......\n");
            }
            conn.Close();
            return;
          }
        } while (!conn.process());

        do {
          int ret = -1;
          int writeErrno = 0;
          ret = conn.write(&writeErrno);
          if (ret <= 0 || errno != EAGAIN) {
            conn.Close();
            return;
          }
        } while (conn.ToWriteBytes() != 0);

      } while (conn.IsKeepAlive());

      // conn.Close();
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