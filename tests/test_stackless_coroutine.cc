#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

#include "coroutine/scheduler.h"

using namespace pio;

class EchoServer {
 public:
  EchoServer(Scheduler* scheduler) : scheduler_(scheduler) {}

  void Run() {
    int listen_fd = CreateSocket();
    BindAndListen(listen_fd);
    scheduler_->AddLazy(AcceptClients(listen_fd).handle(), listen_fd);

    scheduler_->Run();
  }

 private:
  int CreateSocket() {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
      throw std::runtime_error("创建套接字失败");
    }
    return listen_fd;
  }

  void BindAndListen(int listen_fd) {
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(8080);

    if (bind(listen_fd, reinterpret_cast<sockaddr*>(&address),
             sizeof(address)) == -1) {
      throw std::runtime_error("绑定套接字失败");
    }

    if (listen(listen_fd, SOMAXCONN) == -1) {
      throw std::runtime_error("监听套接字失败");
    }

    std::cout << "服务器启动";
  }

  pio::Lazy<void> AcceptClients(int listen_fd) {
    while (true) {
      sockaddr_in client_address{};
      socklen_t client_address_len = sizeof(client_address);
      int client_fd =
          accept(listen_fd, reinterpret_cast<sockaddr*>(&client_address),
                 &client_address_len);
      if (client_fd == -1) {
        throw std::runtime_error("接受客户端连接失败");
      }

      pio::Lazy<void> client_task = HandleClient(client_fd);
      scheduler_->AddLazy(std::move(client_task.handle()),
                          client_fd);  // 使用 scheduler 对象调用 AddLazy 方法
    }

    co_return;
  }

  pio::Lazy<void> HandleClient(int client_fd) {
    char buffer[1024];
    ssize_t num_bytes;

    while ((num_bytes = read(client_fd, buffer, sizeof(buffer))) > 0) {
      write(client_fd, buffer, num_bytes);
    }

    close(client_fd);

    co_return;
  }

  Scheduler* scheduler_;  // 添加一个 Scheduler 对象作为成员变量
};

int main() {
  try {
    Scheduler* scheduler = new Scheduler();
    EchoServer echo_server(scheduler);
    echo_server.Run();
  } catch (const std::exception& e) {
    std::cerr << "错误: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}