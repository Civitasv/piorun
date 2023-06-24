#ifndef PIORUN_COROUTINE_ASYNC_ACCEPT_H_
#define PIORUN_COROUTINE_ASYNC_ACCEPT_H_

#include "coroutine/task/chainable.h"
#include "coroutine/task/terminating.h"
#include "socket.h"

namespace pio {

task::Chainable AsyncAccept(
    SocketView s, std::function<task::Terminating(Socket)> connection_handler);

}

#endif  // PIORUN_COROUTINE_ASYNC_ACCEPT_H_
