#ifndef PIORUN_COROUTINE_ASYNC_WRITE_H_
#define PIORUN_COROUTINE_ASYNC_WRITE_H_

#include <span>
#include "coroutine/task/chainable.h"
#include "socket.h"

namespace pio {

task::Chainable AsyncWrite(SocketView s, std::span<const std::byte> &data);

}

#endif  // PIORUN_COROUTINE_ASYNC_WRITE_H_