#include <string.h>

#include <iostream>
#include <span>

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
#include "socket.h"

using namespace pio;
using namespace std::chrono_literals;

auto main_logger = Logger::Create();
auto client_logger = Logger::Create();
auto server_logger = Logger::Create();

std::span<std::byte> ToReadBuff(auto &data) {
  return std::as_writable_bytes(std::span(data));
}

std::span<const std::byte> ToWriteBuff(auto &data) {
  return std::as_bytes(std::span(data));
}

task::Terminating EchoServerHandle(Socket s) {
  uint32_t rd_size[1] = {0};
  auto rd_buff = ToReadBuff(rd_size);
  if (auto status = co_await AsyncRead(s.WithoutTimeout(), rd_buff);
      status.err != 0)
    throw std::system_error(status.err, std::system_category(),
                            status.err_message);

  server_logger << "Server | read length of request " << rd_size[0] << "\n";

  std::string rd_text(rd_size[0], ' ');
  rd_buff = ToReadBuff(rd_text);
  if (auto status = co_await AsyncRead(s.WithoutTimeout(), rd_buff);
      status.err != 0)
    throw std::system_error(status.err, std::system_category(),
                            status.err_message);

  server_logger << "Server | read content of request \"" << rd_text << "\"\n";

  auto wr_buff = ToWriteBuff(rd_size);
  if (auto status = co_await AsyncWrite(s.WithoutTimeout(), wr_buff);
      status.err != 0)
    throw std::system_error(status.err, std::system_category(),
                            status.err_message);
  server_logger << "Server | wrote response length\n";

  wr_buff = ToWriteBuff(rd_text);
  if (auto status = co_await AsyncWrite(s.WithoutTimeout(), wr_buff);
      status.err != 0)
    throw std::system_error(status.err, std::system_category(),
                            status.err_message);
  server_logger << "Server | wrote response content\n";

  co_return;
}

task::Terminating EchoServerHandle2(Socket s) {
  while (true) {
    char read_buf[2048];
    memset(read_buf, '\0', 2048);
    if (auto status = co_await AsyncRead(s.WithoutTimeout(), read_buf);
        status.err != 0)
      throw std::system_error(status.err, std::system_category(),
                              status.err_message);

    if (auto status =
            co_await AsyncWrite(s.WithoutTimeout(), read_buf, strlen(read_buf));
        status.err != 0) {
      throw std::system_error(status.err, std::system_category(),
                              status.err_message);
    }
  }
}

task::Terminating EchoClient(SocketView server_socket) {
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

  std::string wr_text = "Hello World!";
  uint32_t wr_size[1] = {static_cast<uint32_t>(wr_text.length())};
  auto wr_buff = ToWriteBuff(wr_size);
  if (auto status = co_await AsyncWrite(s.WithoutTimeout(), wr_buff);
      status.err != 0)
    throw std::system_error(status.err, std::system_category(),
                            status.err_message);
  client_logger << "Echo Client | wrote request length\n";

  wr_buff = ToWriteBuff(wr_text);
  if (auto status = co_await AsyncWrite(s.WithoutTimeout(), wr_buff);
      status.err != 0)
    throw std::system_error(status.err, std::system_category(),
                            status.err_message);
  client_logger << "Echo Client | wrote request content\n";

  uint32_t rd_size[1] = {0};
  auto rd_buff = ToReadBuff(rd_size);
  if (auto status = co_await AsyncRead(s.WithoutTimeout(), rd_buff);
      status.err != 0)
    throw std::system_error(status.err, std::system_category(),
                            status.err_message);
  client_logger << "Echo Client | got reponse length " << rd_size[0] << "\n";

  std::string rd_text(rd_size[0], ' ');
  rd_buff = ToReadBuff(rd_text);
  if (auto status = co_await AsyncRead(s.WithoutTimeout(), rd_buff);
      status.err != 0)
    throw std::system_error(status.err, std::system_category(),
                            status.err_message);
  client_logger << "Echo Client | got data \"" << rd_text << "\"\n";

  co_return;
}

int main() {
  auto &sched = MainScheduler();
  sched.RegisterEmitter(CreateScope<emitter::Timeout>());
  sched.RegisterEmitter(CreateScope<emitter::Condition>());
  sched.RegisterEmitter(CreateScope<emitter::Epoll>());

  Socket s = Socket::ServerSocket(SockAddr(IPv4{}, "127.0.0.1", 9090));
  auto server = AsyncServer(s.WithoutTimeout(), EchoServerHandle2);
  // auto client = EchoClient(s.WithoutTimeout());

  sched.Schedule(server);
  // sched.Schedule(client);

  sched.Run();

  main_logger->Info("Main done");
}