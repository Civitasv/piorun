// #include <fcntl.h>
// #include <netinet/in.h>
// #include <sys/socket.h>
// #include <unistd.h>

#include <coroutine>
#include <iostream>
#include <vector>

#include "coroutine/generator.h"
#include "coroutine/scheduler.h"
#include "coroutine/task.h"

using namespace pio;

Generator<int> Interleaved(std::vector<int> a, std::vector<int> b) {
  auto lamb = [](std::vector<int>& v) -> Generator<int> {
    for (const auto& e : v) {
      co_yield e;
    }
  };

  auto x = lamb(a);
  auto y = lamb(b);

  while (not x.Finished() or not y.Finished()) {
    if (not x.Finished()) {
      co_yield x.Value();
      x.Resume();
    }

    if (not y.Finished()) {
      co_yield y.Value();
      y.Resume();
    }
  }
}

Task TaskA(Scheduler& sched) {
  std::cout << "Hello, from task A\n";

  co_await sched.Suspend();

  std::cout << "A is back doing work\n";

  co_await sched.Suspend();

  std::cout << "A is back doing more work\n";
}

Task TaskB(Scheduler& sched) {
  std::cout << "Hello, from task B\n";

  co_await sched.Suspend();

  std::cout << "B is back doing work\n";

  co_await sched.Suspend();

  std::cout << "B is back doing more work\n";
}

void SchedulerUsage() {
  Scheduler sched;

  TaskA(sched);
  TaskB(sched);

  while (sched.Schedule()) {
  }
}

// #include "coroutine/scheduler.h"

// using namespace pio;

// class EchoServer {
//  public:
//   EchoServer(Scheduler* scheduler) : scheduler_(scheduler) {}

//   void Run() {
//     int listen_fd = CreateSocket();
//     BindAndListen(listen_fd);
//     scheduler_->AddLazy(AcceptClients(listen_fd).handle(), listen_fd);

//     scheduler_->Run();
//   }

//  private:
//   int CreateSocket() {
//     int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
//     if (listen_fd == -1) {
//       throw std::runtime_error("创建套接字失败");
//     }
//     return listen_fd;
//   }

//   void BindAndListen(int listen_fd) {
//     sockaddr_in address{};
//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = htonl(INADDR_ANY);
//     address.sin_port = htons(8080);

//     if (bind(listen_fd, reinterpret_cast<sockaddr*>(&address),
//              sizeof(address)) == -1) {
//       throw std::runtime_error("绑定套接字失败");
//     }

//     if (listen(listen_fd, SOMAXCONN) == -1) {
//       throw std::runtime_error("监听套接字失败");
//     }

//     scheduler_->listen_fd_ = listen_fd;
//   }

//   pio::Lazy<void> AcceptClients(int listen_fd) {
//     while (true) {
//       // co_await std::suspend_always{};
//       sockaddr_in client_address{};
//       socklen_t client_address_len = sizeof(client_address);
//       int client_fd =
//           accept(listen_fd, reinterpret_cast<sockaddr*>(&client_address),
//                  &client_address_len);
//       if (client_fd == -1) {
//         throw std::runtime_error("接受客户端连接失败");
//       }

//       int flag = fcntl(client_fd, F_GETFL);
//       flag |= O_NONBLOCK;
//       fcntl(client_fd, F_SETFL, flag);

//       scheduler_->AddLazy(HandleClient(client_fd).handle(),
//                           client_fd);  // 使用 scheduler 对象调用 AddLazy
//                           方法
//     }

//     co_return;
//   }

//   pio::Lazy<void> HandleClient(int client_fd) {
//     char buffer[1024];
//     ssize_t num_bytes;

//     while (1) {
//       num_bytes = read(client_fd, buffer, sizeof(buffer));
//       if (num_bytes == 0) {
//         epoll_ctl(scheduler_->epoll_fd_, EPOLL_CTL_DEL, client_fd, NULL);
//         close(client_fd);
//         co_return;
//       } else if (num_bytes > 0) {
//         write(client_fd, buffer, num_bytes);
//       } else {
//         if (errno == EAGAIN || errno == EWOULDBLOCK) {
//           co_await std::suspend_always{};
//         } else {
//           logger->Error("read client fd error");
//           exit(0);
//         }
//       }
//     }

//     co_return;
//   }

//   Scheduler* scheduler_;  // 添加一个 Scheduler 对象作为成员变量
// };

int main() {
#if 0  // generator example
  std::vector<int> a = {1, 3, 5, 7};
  std::vector<int> b = {2, 4, 6, 8};
  auto generator = Interleaved(a, b);

  for (const auto& e : generator) {
    std::cout << e << '\n';
  }
#endif

  SchedulerUsage();

  return 0;
}