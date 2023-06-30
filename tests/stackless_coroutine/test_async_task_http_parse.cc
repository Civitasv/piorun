#include <string.h>

#include <iostream>

#include "core/smartptr.h"
#include "coroutine/async/async_connect.h"
#include "coroutine/async/async_read.h"
#include "coroutine/async/async_server.h"
#include "coroutine/async/async_write.h"
#include "coroutine/emitter/condition.h"
#include "coroutine/emitter/epoll.h"
#include "coroutine/emitter/timeout.h"
#include "coroutine/scheduler.h"
#include "coroutine/task/terminating.h"
#include "http/http_conn.h"
#include "socket.h"

using namespace pio;

auto main_logger = Logger::Create();
auto client_logger = Logger::Create();
auto server_logger = Logger::Create();

task::Terminating HttpServerHandle(Socket s) {
  HttpConn conn = HttpConn();
  conn.init();
  while (true) {
    if (auto status = co_await AsyncRead(s.WithoutTimeout(), conn.read_buf_);
        status.err != 0)
      throw std::system_error(status.err, std::system_category(),
                              status.err_message);
    conn.Process();
    if (auto status = co_await AsyncWrite(s.WithoutTimeout(), conn.write_buf_,
                                          strlen(conn.write_buf_));
        status.err != 0) {
      throw std::system_error(status.err, std::system_category(),
                              status.err_message);
    }
    conn.init();
  }
}

task::Terminating HttpClientHandle(SocketView server_socket) {
  if (auto status = co_await MainScheduler().Condition(
          [server_socket]() { return server_socket->operational_; });
      !status)
    throw std::system_error(status.err, std::system_category(),
                            status.err_message);

  Socket s = server_socket->MakeClient();
  if (auto status = co_await AsyncConnect(s.WithoutTimeout());
      status.err != 0) {
    throw std::system_error(status.err, std::system_category(),
                            status.err_message);
  }

  std::string wr_text =
      "GET / HTTP/1.1\r\nUser-Agent: Mozilla/4.0 (compatible; MSIE5.01; "
      "Windows NT)\r\nHost: www.tutorialspoint.com\r\nAccept-Language: "
      "en-us\r\nAccept-Encoding: gzip, deflate\r\nConnection: Keep-Alive\r\n";
  char wr_buf[1024];
  strcpy(wr_buf, wr_text.c_str());
  if (auto status =
          co_await AsyncWrite(s.WithoutTimeout(), wr_buf, strlen(wr_buf));
      status.err != 0)
    throw std::system_error(status.err, std::system_category(),
                            status.err_message);

  char rd_buf[1024];
  if (auto status = co_await AsyncRead(s.WithoutTimeout(), rd_buf);
      status.err != 0)
    throw std::system_error(status.err, std::system_category(),
                            status.err_message);

  client_logger << rd_buf;
  co_return;
}

int main() {
  auto& sched = MainScheduler();
  sched.RegisterEmitter(CreateScope<emitter::Timeout>());
  sched.RegisterEmitter(CreateScope<emitter::Condition>());
  sched.RegisterEmitter(CreateScope<emitter::Epoll>());

  Socket s = Socket::ServerSocket(SockAddr(IPv4{}, "127.0.0.1", 1234));
  auto server = AsyncServer(s.WithoutTimeout(), HttpServerHandle);
  // auto client = HttpClientHandle(s.WithoutTimeout());

  sched.Schedule(server);
  // sched.Schedule(client);

  sched.Run();
}