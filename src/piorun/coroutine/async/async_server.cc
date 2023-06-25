#include "coroutine/async/async_server.h"

#include "coroutine/async/async_accept.h"
#include "coroutine/awaitable/event.h"
#include "coroutine/task/chainable.h"
#include "socket.h"

namespace pio {

task::Chainable AsyncServer(
    SocketView s, std::function<task::Terminating(Socket)> connection_handler) {
  while (true) {
    if (auto result = co_await AsyncAccept(s, connection_handler); !result)
      co_return result;
  }
}

}  // namespace pio
