#include <iostream>
#include <map>
#include <fstream>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/poll.h>
#include <arpa/inet.h>

#include <fiber/fiber.h>

using namespace pio;
using namespace fiber;
using namespace this_fiber;

static void LogInfo(const std::string& msg) {
  struct tm t;
  struct timeval now = {0, 0};
  gettimeofday(&now, nullptr);
  time_t tsec = now.tv_sec;
  localtime_r(&tsec, &t);
  printf (
    "%d-%02d-%02d %02d:%02d:%02d.%03ld [info] : [%d:%p]: %s\n", 
    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
    t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec / 1000,
    get_thread_id(), co_self(), msg.c_str()
  );
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
    int nReuseAddr = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &nReuseAddr, sizeof(nReuseAddr));
	}
	return fd;
}

class Server;

class User : public std::enable_shared_from_this<User> {

friend class Server;

 public:
  User(int connfd, Server* server, sockaddr_in addr)
  : connfd(connfd), isClosed(0), server(server), pipe(1) {
    char* ipAddr = new char[32];
    memset(ipAddr, 0, 32);
    sprintf(ipAddr, "%s:%d", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    Addr = ipAddr;
    Name = Addr;
    delete[] ipAddr;

    go std::bind(&User::ListenMessage, this);
  }

 ~User() {
    close(connfd);
  }

  void Online();
  void Offline();
  void ListenMessage();
  void DoMessage(const std::string& msg);
  void SendMsg(const std::string& msg);

 private:
  int connfd;
  int isClosed;
  std::string Addr;
  std::string Name;
  Server* server;
  channel<std::string> pipe;
  semaphore listenMessageFinished;
};

class Server {

friend class User;

 public:
  Server() : Message(1) {}
 ~Server() {}

  void start();
  void stop();
  void listenMessager();
  void BroadCast(User* user, const std::string& msg);
  void handler(int clifd, sockaddr_in addr);

 private:
  std::string IP;
  int Port;
  std::atomic<int> isClosed;
  std::map<std::string, std::shared_ptr<User>> OnlineMap;
  shared_mutex mapLock;
  channel<std::string> Message;
  semaphore listenMessagerFinished;

};

void Server::start() {
  int listenfd = CreateTcpSocket(12345, "127.0.0.1");
  assert(listenfd >= 0);
  SetNonBlock(listenfd);
  listen(listenfd, 1024);

  go std::bind(&Server::listenMessager, this);

  while (!isClosed) {
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
  this->Message.write("");
  listenMessagerFinished.wait();
}

void Server::stop() {
  isClosed = 1;
}

void Server::listenMessager() {
  while (true) {
    this->mapLock.lock_shared();
    if (isClosed && OnlineMap.empty()) {
      this->mapLock.unlock_shared();
      break;
    }
    this->mapLock.unlock_shared();
    std::string msg = this->Message.read();
    if (msg.size() <= 0) continue;
    this->mapLock.lock_shared();
    for (auto&& x : OnlineMap) {
      x.second->pipe.write(msg);
    }
    this->mapLock.unlock_shared();
  }
  listenMessagerFinished.signal();
}

void Server::handler(int clifd, sockaddr_in addr) {

  std::shared_ptr user = std::make_shared<User>(clifd, this, addr);
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

}

void Server::BroadCast(User* user, const std::string& msg) {
  std::string sendMsg = "[" + user->Addr + "]" + user->Name + ":" + msg;
  this->Message.write(std::move(sendMsg));
}

void User::ListenMessage() {
  while (!isClosed) {
    std::string msg = this->pipe.read();
    if (msg.size() > 0) {
      write(this->connfd, msg.c_str(), msg.size());
    }
  }
  listenMessageFinished.signal();
}

void User::Online() {
  this->server->mapLock.lock();
  this->server->OnlineMap[this->Name] = shared_from_this();
  this->server->mapLock.unlock();
  LogInfo(this->Addr + " login");
  this->server->BroadCast(this, "已上线\n");
}

void User::Offline() {
  this->server->mapLock.lock();
  this->server->OnlineMap.erase(this->Name);
  this->server->mapLock.unlock();
  LogInfo(this->Addr + " logout");
  this->server->BroadCast(this, "下线\n");
  this->isClosed = 1;
  this->pipe.write(""); // resume Routine: ListenMessage.
  this->listenMessageFinished.wait();
}

void User::DoMessage(const std::string& msg) {
  
  if (msg == "who\n") {
    std::string onlineMsg;
    this->server->mapLock.lock_shared();
    for (auto&& usr : this->server->OnlineMap) {
      onlineMsg += "[" + usr.second->Addr + "]" + usr.second->Name + ":" + "在线...\n";
    }
    this->server->mapLock.unlock_shared();
    this->SendMsg(onlineMsg);
  } 
  
  else if (msg.size() > 7 && strncmp(msg.c_str(), "rename|", 7) == 0) {
    std::string newName = msg.substr(7, msg.size() - 8);
    this->server->mapLock.lock_shared();
    if (this->server->OnlineMap.find(newName) != this->server->OnlineMap.end()) {
      this->server->mapLock.unlock_shared();
      this->SendMsg("当前用户名被使用\n");
    } 
    else {
      this->server->mapLock.unlock_shared();
      this->server->mapLock.lock();
      this->server->OnlineMap.erase(this->Name);
      this->server->OnlineMap[newName] = shared_from_this();
      this->server->mapLock.unlock();
      this->Name = std::move(newName);
      this->SendMsg("您已经更新用户名:" + this->Name + "\n");
    }
  } 
  
  else if (msg.size() > 4 && strncmp(msg.c_str(), "to|", 3) == 0) {
    std::string remoteName = msg.substr(msg.find_first_of("|") + 1, msg.find_last_of("|") - msg.find_first_of("|") - 1);
    this->server->mapLock.lock_shared();
    auto remoteUser = this->server->OnlineMap.find(remoteName);
    if (remoteUser == this->server->OnlineMap.end()) {
      this->server->mapLock.unlock_shared();
      this->SendMsg("该用户名不存在\n");
      return;
    } else {
      auto remote = remoteUser->second;
      this->server->mapLock.unlock_shared();
      std::string content = msg.substr(msg.find_last_of("|") + 1);
      if (content == "") {
        this->SendMsg("无消息内容，请重发\n");
        return;
      }
      remote->SendMsg(this->Name + "对您说:" + content);
    }
  } else {
    this->server->BroadCast(this, msg);
  }
}

void User::SendMsg(const std::string& msg) {
  write(this->connfd, msg.c_str(), msg.size());
}

Server server;
const char* szPidFile = "imsystem.pid";

static void sighdr(int x) {
  server.stop();
  unlink(szPidFile);
}

static void writePidFile() {
  /* 获取文件描述符 */
  char str[32];
  int pidfile = open(szPidFile, O_WRONLY|O_CREAT, 0600);
  if (pidfile < 0) exit(1);
  
  /* 锁定文件，如果失败则说明文件已被锁，存在一个正在运行的进程，程序直接退出 */
  if (lockf(pidfile, F_TLOCK, 0) < 0) {
    printf("already running\n");
    exit(0);
  }

  /* 锁定文件成功后，会一直持有这把锁，知道进程退出，或者手动 close 文件
      然后将进程的进程号写入到 pid 文件*/
  ftruncate(pidfile, 0);
  sprintf(str, "%d\n", getpid()); // \n is a symbol.
  ssize_t len = strlen(str);
  ssize_t ret = write(pidfile, str, len);
  if (ret != len ) {
    fprintf(stderr, "Can't Write Pid File: %s", szPidFile);
    exit(0);
  }
}

int main(int argc, char *argv[]) {

  if (argc > 1 && strcmp(argv[1], "-s") == 0) {
    std::ifstream file(szPidFile);
    int pid;
    if (file.is_open()) {
      file >> pid;
      file.close();
      assert(pid > 1);
      kill(pid, SIGQUIT);
    }
    return 0;
  }

  daemon(1, 1);
  signal(SIGQUIT, sighdr);
  writePidFile();
  
  go std::bind(&Server::start, &server);

  return 0;

}