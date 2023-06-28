#include <iostream>
#include <map>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/poll.h>
#include <arpa/inet.h>

#include <fiber/fiber.h>

using namespace pio;
using namespace fiber;
using namespace this_fiber;

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

class Server;

class User {

friend class Server;

 public:
  User(int connfd, Server* server, sockaddr_in addr)
  : connfd(connfd), server(server), pipe(1) {
    char* ipAddr = new char[32];
    memset(ipAddr, 0, 32);
    sprintf(ipAddr, "%s:%d", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    Addr = ipAddr;
    Name = Addr;
    delete[] ipAddr;

    go std::bind(&User::ListenMessage, this);
  }

  void Online();
  void Offline();
  void ListenMessage();
  void DoMessage(const std::string& msg);
  void SendMsg(const std::string& msg);

 private:
  int connfd;
  std::string Addr;
  std::string Name;
  Server* server;
  channel<std::string> pipe;

};

class Server {

friend class User;

 public:
  Server() : Message(1) {}
 ~Server() {}

  void start();
  void listenMessager();
  void BroadCast(User* user, const std::string& msg);
  void handler(int clifd, sockaddr_in addr);

 private:
  std::string IP;
  int Port;
  std::map<std::string, User*> OnlineMap;
  shared_mutex mapLock;
  channel<std::string> Message;

};

void Server::start() {
  int listenfd = CreateTcpSocket(12345, "127.0.0.1");
  assert(listenfd >= 0);
  SetNonBlock(listenfd);
  listen(listenfd, 1024);

  go std::bind(&Server::listenMessager, this);

  for (;;) {
    sockaddr_in cliAddr;
    socklen_t cliAddrLen = sizeof(cliAddr);
    int clifd = accept(listenfd, (sockaddr*)(&cliAddr), &cliAddrLen);
    if (clifd < 0) {
      struct pollfd pf = { 0 };
      pf.fd = listenfd;
      pf.events = (POLLIN | POLLERR | POLLHUP);
      poll(&pf, 1, 1000);
      continue;
    }
    go std::bind(&Server::handler, this, clifd, cliAddr);
  }

  close(listenfd);
}

void Server::listenMessager() {
  for (;;) {
    std::string msg = this->Message.read();
    this->mapLock.lock();
    for (auto&& x : OnlineMap) {
      x.second->pipe.write(msg);
    }
    this->mapLock.unlock();
  }
}

void Server::handler(int clifd, sockaddr_in addr) {

  User* user = new User(clifd, this, addr);
  user->Online();

  char* buf = new char[4096];
  timeval timeout;
  timeout.tv_sec = 60;
  timeout.tv_usec = 0;
  setsockopt(clifd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  for (;;) {
    memset(buf, 0, 4096);
    int n = read(clifd, buf, 4096);
    if (n == 0) {
      user->Offline();
      break;
    }
    if (n < 0 && errno == EAGAIN) {
      user->SendMsg("你被踢了\n");
      user->Offline();
      break;
    }
    if (n > 0) {
      user->DoMessage(buf);
    }
  }
  delete[] buf;
  delete user;
  close(clifd);

}

void Server::BroadCast(User* user, const std::string& msg) {
  std::string sendMsg = "[" + user->Addr + "]" + user->Name + ":" + msg;
  this->Message.write(std::move(sendMsg));
}

void User::ListenMessage() {
  for (;;) {
    std::string msg = this->pipe.read();
    write(this->connfd, msg.c_str(), msg.size());
  }
}

void User::Online() {
  this->server->mapLock.lock();
  this->server->OnlineMap[this->Name] = this;
  this->server->mapLock.unlock();
  this->server->BroadCast(this, "已上线\n");
}

void User::Offline() {
  this->server->mapLock.lock();
  this->server->OnlineMap.erase(this->Name);
  this->server->mapLock.unlock();
  this->server->BroadCast(this, "下线\n");
}

void User::DoMessage(const std::string& msg) {
  if (msg == "who\n") {
    this->server->mapLock.lock();
    for (auto&& usr : this->server->OnlineMap) {
      std::string onlineMsg = "[" + usr.second->Addr + "]" + usr.second->Name + ":" + "在线...\n";
      this->SendMsg(onlineMsg);
    }
    this->server->mapLock.unlock();
  } else if (msg.size() > 7 && strncmp(msg.c_str(), "rename|", 7) == 0) {
    std::string newName = msg.substr(7, msg.size() - 8);
    this->server->mapLock.lock();
    if (this->server->OnlineMap.find(newName) != this->server->OnlineMap.end()) {
      this->server->mapLock.unlock();
      this->SendMsg("当前用户名被使用\n");
    } else {
      this->server->OnlineMap.erase(this->Name);
      this->server->OnlineMap[newName] = this;
      this->server->mapLock.unlock();
      this->Name = std::move(newName);
      this->SendMsg("您已经更新用户名:" + this->Name + "\n");
    }
  } else if (msg.size() > 4 && strncmp(msg.c_str(), "to|", 3) == 0) {
    std::string remoteName = msg.substr(msg.find_first_of("|") + 1, msg.find_last_of("|") - msg.find_first_of("|") - 1);
    this->server->mapLock.lock();
    auto remoteUser = this->server->OnlineMap.find(remoteName);
    if (remoteUser == this->server->OnlineMap.end()) {
      this->server->mapLock.unlock();
      this->SendMsg("该用户名不存在\n");
      return;
    } else {
      std::string content = msg.substr(msg.find_last_of("|") + 1);
      if (content == "") {
        this->server->mapLock.unlock();
        this->SendMsg("无消息内容，请重发\n");
        return;
      }
      remoteUser->second->SendMsg(this->Name + "对您说:" + content);
    }
    this->server->mapLock.unlock();
  } else {
    this->server->BroadCast(this, msg);
  }
}

void User::SendMsg(const std::string& msg) {
  write(this->connfd, msg.c_str(), msg.size());
}

Server server;

int main(int argc, char *argv[]) {
  
  go std::bind(&Server::start, &server);

  return 0;

}