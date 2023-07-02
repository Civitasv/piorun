#include <iostream>
#include <functional>

#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "fiber/fiber.h"

using namespace pio;
using namespace pio::fiber;
using namespace std::chrono;

int stop = 0;

static void sighdr(int x) {
  stop = 1;
}

static int SetNonBlock(int iSock) {
  int iFlags;

  iFlags = fcntl(iSock, F_GETFL, 0);
  iFlags |= O_NONBLOCK;
  iFlags |= O_NDELAY;
  int ret = fcntl(iSock, F_SETFL, iFlags);
  return ret;
}

static void SetAddr(const char *pszIP, const unsigned short shPort, sockaddr_in &addr) {
	bzero(&addr,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(shPort);
	int nIP = 0;
	if (!pszIP || '\0' == *pszIP   
	    || 0 == strcmp(pszIP,"0") || 0 == strcmp(pszIP,"0.0.0.0") 
		  || 0 == strcmp(pszIP,"*")) {
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
    SetAddr(pszIP,shPort,addr);
    int ret = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    if ( ret != 0) {
      close(fd);
      return -1;
    }
	}
	return fd;
}

void EchoServer() {

  int listenfd = CreateTcpSocket(12345, "127.0.0.1");
  SetNonBlock(listenfd);
  listen(listenfd, 1024);

  assert(listenfd >= 0);

  while (!stop) {
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    
    int clifd = accept(listenfd, (sockaddr*)&addr, &len);
    
    if (clifd < 0) {
      // 因为listenfd是非阻塞的，如果accept失败，使用poll监视listenfd是否有新连接可accept.
      // 这里不能使用epoll，使用libco后用户不能随意使用epoll.
      struct pollfd pf = { 0 };
      pf.fd = listenfd;
      pf.events = (POLLIN | POLLERR | POLLHUP);
      poll(&pf, 1, 1000);
      continue;
    } else {
      char buf[32];
      memset(buf, 0, 32);
      inet_ntop(AF_INET, &addr.sin_addr, buf, 32);
      printf("%s\n", buf);
    }

    go [clifd] {
      printf("new session at: [%d|%p]\n", this_fiber::get_thread_id(), this_fiber::co_self());
      char buf[1024];
      while (true) {
        memset(buf, 0, sizeof(buf));
        int n = read(clifd, buf, sizeof(buf));
        if (n == 0) {
          close(clifd);
          break;
        }
        write(clifd, buf, n);
      }
    };
  }

  close(listenfd);

}

int main(int argc, const char* agrv[]) {

  signal(SIGQUIT, sighdr);

  go EchoServer;

  return 0;

}
