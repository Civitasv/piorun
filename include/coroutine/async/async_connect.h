#ifndef PIORUN_COROUTINE_ASYNC_CONNECT_H_
#define PIORUN_COROUTINE_ASYNC_CONNECT_H_

#include "coroutine/task/chainable.h"
#include "socket.h"

namespace pio {

task::Chainable AsyncConnect(SocketView s);

}

#endif  // PIORUN_COROUTINE_ASYNC_CONNECT_H_